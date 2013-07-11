# Kawa

Use libuv inside PHP

## How to build the damn thing (on Linux)

Right now building the extension is kinda involved:
  1. Run ```git submodule init``` and ```git submodule update``` to retrieve libuv
  2. Go to the libuv directory
  3. If using a recent version of automake (1.14+), add **AUTOMAKE_OPTIONS = subdir-objects** in ***autogen.sh***
  4. Run ```./autogen.sh```
  5. Edit ***Makefile*** and add **-fPIC** to the **CFLAGS** definition
  6. Run ```./configure``` and ```make```
  7. Go back to the main directory then in the src/ directory
  8. Run ```phpize```, ```./configure``` then ```make```
  9. Tada!

The reason you have to do all that is that right now, almost no distro provides an up to date version of libuv. I will most likely add support for the .so files later but I'm against any kind of ```make install``` that is not done through a package manager.

## How to run some code

At that point the extension is built, you can edit and run the ***test.php*** by calling:
```php -n -d "extension_dir=modules/" -d "extension=kawa.so" test.php```
This test script launch a server on port 7000 that will dump any message sent to it before closing the connection.
The code is self explanatory:
* __Pool__ is a container for fds, you can run separate pools with each their own sockets, when calling ```$pool->run();``` it will start a loop that ends only when every fd/streams/sockets have been closed.
* __TCP__ is a TCP connection, or a server, API is kinda empty right now

__/!\ It is not recommended to run this in Apache or any kind of web server!!!__

## API

Right now I'm still working on how I want to call everything, libuv is pretty huge (see https://www.dropbox.com/s/dybwwyi9rs9zo97/Libuv.png) and I want to provide something as close as possible as the original API.

## License

MIT

## Bugs

Yep, right now it's in development
