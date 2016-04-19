# CryptRelay
CryptRelay is a command line peer-to-peer encrypted chat and file transfer program for Windows and Linux. Currently a work in progress.

CryptRelay utilizes UPnP (Universal Plug and Play) to port forward so the user doesn't have to do it manually.

Programmed in Netbeans IDE 8.1 when using Debian Linux (not something I can recommend), and Microsoft Visual Studio 2015 when using Windows 7.
C++14.

#External Libraries:
miniupnp v1.9 - LICENSE here - for help building this library on Windows, please look at README_miniupnp.md




In order to compile/build CryptRelay on linux, you must set the linker library to the standard library named Posix Threads.
An example using Netbeans 8.1:
file->project properties->build->linker->libraries-> Add Standard Library-> Posix Threads
