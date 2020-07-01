Uppaal DBM Library
==================

How to compile:
---------------
```sh
./configure
make
```

The `configure` script can be regenerated using locally installed `autotools` by running `./autogen.sh` as the first command.

How to install:
---------------
```sh
sudo make install
```

How to compile using the configure front-end:
---------------------------------------------
```sh
./setup.sh
make
```

Typical configuration:
----------------------
```sh
CFLAGS="-Wall -O2 -DNDEBUG" ./configure --prefix=/usr/local
```

Debug configuration:
--------------------
```sh
CFLAGS="-Wall -g" ./configure
```
