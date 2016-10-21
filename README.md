# Symphony FTP Demo
This is a refrence application to show the user how to handle a File Tranfer from the server, to the module (client) through
the an external host. In this application, we use a computer as an external host to connect to a Module in order to complete
a file transfer.

## Quick-Start
Make sure you clone git recursively in order to download the host-interface library.
```
git clone --recursive
```
If you did not clone recursively then you can pull the git submodule with `git submodule update --init --recursive`.
Now we are going to create a build directory.
```
mkdir build
cd build
```
Using CMake, we're going to generate the Makefiles and make the binary.
```
cmake ..
make
```

## In-Depth Guide and Documentation
Checkout [docs.link-labs.com](http://docs.link-labs.com/m/52162/c/184122).
