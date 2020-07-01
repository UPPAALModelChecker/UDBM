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

#ifndef INCLUDE_IO_CONNECTION_H
#define INCLUDE_IO_CONNECTION_H

#include <stdlib.h>
#include <string>

struct sockaddr;

namespace io
{
    /**
     * Create a network connection to a given host:port.
     * The host may be given as an ip (string) or host name.
     * This is a base class for use in different contexts so
     * it does not support exception for Windows/thread
     * compatibility reasons. Error handling is also
     * limited with a few codes defined here for compatibility
     * problems with Windows (errno not used in the same way).
     */
    class Connection
    {
    public:
        /** Create the connection. The connection
         * may fail, in which case it is closed.
         */
        Connection(const char* adr, int port);

        /** Call close() if the connection is not
         * already closed.
         */
        ~Connection();

        /** @return true if the connection is open.
         */
        bool isOpen() const;

        /** Close the current connection. Closing a
         * closed connection has no effect.
         */
        void close();

        /** Re-connect to the previous address:port,
         * typically useful after a close(). If the
         * current connection is not closed then close()
         * is called before reconnecting.
         * @return true if successful, false otherwise.
         */
        bool reconnect();
        
        /** Read an object from this connection.
         * @return true if successful, false otherwise.
         */
        template<typename T>
        bool read(T&);

        /** Write an object to this connection.
         * @return true if successful, false otherwise.
         */
        template<typename T>
        bool write(const T&);

        /** Specialized version for std::string.
         */
        bool write(const std::string&);

        /** Read from this connection. The data is read
         * to an internally managed buffer whose address
         * is returned.
         * @return NULL if failed or a non-NULL pointer
         * otherwise. The string is read until EOF. The
         * size read is returned in len if len != NULL
         * and the string is '\0' terminated.
         */
        const char* read(size_t *len = NULL);

        /** Read count bytes into buffer. WARNING: buffer
         * MUST be at least a char[count] buffer. If len != NULL
         * the actual number of read bytes is returned.
         * @return true if count bytes were read, false otherwise.
         */
        bool read(char* buffer, size_t count, size_t *len = NULL);

        /** Write count bytes from buffer to this connection.
         */
        bool write(const char* buffer, size_t count);

        /** Set TCP option for low latency.
         * @return true if successful, false otherwise.
         */
        bool setLowLatency();

        /** Reset the internal buffer to a given size.
         * The internal buffer is re-allocated so you are
         * not supposed to keep a reference to the old one.
         * This can be used conveniently to deallocate
         * memory with resetBuffer() if something large
         * was read before. If size < 16 then the call is
         * ignored.
         */
        void resetBuffer(size_t size = 16);

        const char* getStatus(){ return status; }

    protected:
        /** Re-open the socket. If the socket was open, then
         * it is closed before being re-opened.
         * @return true if the socket is open.
         */
        bool open();

        int socketfd;
        size_t bufferSize;
        char* buffer;
        const char* status;

        // This is implemented as sockaddr_in whose definition
        // is platform dependent. We do not pollute the declaration
        // with that.
        struct sockaddr *address;
    };

    
    /*****************************
     * Template implementations. *
     *****************************/

    template<typename T>
    bool Connection::read(T& obj)
    {
        size_t size;
        return read((char*)obj, sizeof(obj), &size) &&
            size == sizeof(obj);
    }

    template<typename T>
    bool Connection::write(const T& obj)
    {
        return write((const char*)obj, sizeof(obj));
    }

    /*********************************
     * Simple inline implementation. *
     *********************************/

    inline
    bool Connection::isOpen() const
    {
        return socketfd >= 0;
    }

    inline
    bool Connection::write(const std::string& str)
    {
        return write(str.c_str(), str.length());
    }
}

#endif // INCLUDE_IO_CONNECTION_H
