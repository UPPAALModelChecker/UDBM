/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; -*-
 *
 * This file is part of the UPPAAL DBM library.
 *
 * The UPPAAL DBM library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * The UPPAAL DBM library is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with the UPPAAL DBM library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA.
 */

// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
////////////////////////////////////////////////////////////////////
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2006, Uppsala University and Aalborg University.
// All right reserved.
//
///////////////////////////////////////////////////////////////////

#ifdef __MINGW32__

#include <iostream>
#include <winsock2.h>

#define HOW_SHUTDOWN SD_BOTH

static inline
int close(SOCKET s)
{
    return closesocket(s);
}

// Windoz needs some static initialization/destruction.

class WindozData
{
public:
    WindozData()
    {
        WSADATA data;
        bool init = WSAStartup(MAKEWORD(2,2), &data) == 0;
        if (!init || LOBYTE(data.wVersion) != 2 || HIBYTE(data.wVersion) != 2 )
        {
            if (init)
            {
                WSACleanup();
            }
            std::cerr << "Windows socket library version 2.2 not available.\n";
            exit(EXIT_FAILURE);
        }
#ifdef SUPER_VERBOSE
        std::cerr << "Incredible: Winsocket is initialized!\n";
#endif
    }

    ~WindozData()
    {
        WSACleanup( );
    }
};

static WindozData staticCrap;

#else /* assume Unix */

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>

#define HOW_SHUTDOWN SHUT_RDWR

#endif

#include <string.h>
#include <errno.h>
#include "io/Connection.h"

namespace io
{
    Connection::Connection(const char* hostname, int port)
        : socketfd(-1),
          bufferSize(16),
          buffer(new char[16]),
          status(NULL),
          address(NULL)
    {
        if (hostname && port >= 0)
        {
#ifdef __MINGW32__
            // Dirty deprecated interface for Windoz.
            unsigned long addr = inet_addr(hostname);
            struct hostent *hp = addr == INADDR_NONE
                ? gethostbyname(hostname)
                : gethostbyaddr((const char*)&addr, sizeof(addr), AF_INET);
#else
            // Clean interface for Linux.
            in_addr addr;
            struct hostent *hp = inet_aton(hostname, &addr)
                ? gethostbyaddr(&addr, sizeof(addr), AF_INET)
                : gethostbyname(hostname);
#endif

            if (hp)
            {
                struct sockaddr_in addr;
                memset(&addr, 0, sizeof(struct sockaddr_in));
                memcpy(&addr.sin_addr, hp->h_addr, hp->h_length);
                addr.sin_port = htons(port);
                addr.sin_family = AF_INET;
                // Sorry for that:
                address = (struct sockaddr*) new struct sockaddr_in;
                memcpy(address, &addr, sizeof(struct sockaddr_in));
                reconnect();
            } else {
#ifdef __MINGW32__
                status = "Network error";
#else
                status = hstrerror(h_errno);
#endif
            }
        }
    }

    Connection::~Connection()
    {
        close();
        delete [] buffer;
        delete address;
    }

    void Connection::close()
    {
        if (isOpen())
        {
            shutdown(socketfd, HOW_SHUTDOWN);
            ::close(socketfd);
            socketfd = -1;
        }
    }

    bool Connection::open()
    {
        close();
        socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (socketfd == -1) 
        {
            status = strerror(errno);
        }
        return isOpen();
    }

    bool Connection::reconnect()
    {
        if (!address || !open())
        {
            return false;
        }
        if (connect(socketfd, address, sizeof(struct sockaddr_in)))
        {
            close(); // Close the socket on connection errors.
        } else {
            status = strerror(errno);
        }
        return isOpen();
    }

    const char* Connection::read(size_t *len)
    {
        if (!isOpen())
        {
            return NULL;
        }

        size_t size = 0;
        size_t part;
        while((part = recv(socketfd, buffer + size, bufferSize - size - 1, 0)) > 0)
        {
            char *larger = new char[bufferSize << 1];
            strncpy(larger, buffer, bufferSize);
            delete [] buffer;
            buffer = larger;
            size += part;
            bufferSize <<= 1;
        }
        if (part < 0)
        {
            status = strerror(errno);
            return NULL;
        }
        buffer[size] = 0;
        if (len)
        {
            *len = size;
        }
        return buffer;
    }

    bool Connection::read(char* buf, size_t count, size_t *len)
    {
        if (!isOpen())
        {
            return false;
        }
        size_t size = recv(socketfd, buf, count, 0);
        if (size <= 0)
        {
            if (size < 0) status = strerror(errno);
            return false;
        }
        if (len)
        {
            *len = size;
        }
        return true;
    }

    bool Connection::write(const char* buf, size_t count)
    {
        if (!isOpen())
        {
            return false;
        }
        ssize_t result = send(socketfd, buf, count, 0);
        if (result < 0) status = strerror(errno);
        return  (result == (int) count);
    }

    bool Connection::setLowLatency()
    {
        int flag = 1;
        if (!isOpen()) 
        {
            return false;
        } 
        else 
        {
            int result = setsockopt(socketfd, IPPROTO_TCP, TCP_NODELAY,
                                    (const char*)&flag, // cast for Windoz
                                    sizeof(int));
            return (result == 0);
        }
    }

    void Connection::resetBuffer(size_t size)
    {
        if (size >= 16)
        {
            delete [] buffer;
            buffer = new char[size];
            bufferSize = size;
        }
    }

}

