# Compiling a plugin

Plugins for CConArtist are compiled as dynamic libraries. Below is an example command to compile a simple plugin on macOS using `clang`. Adjust the command as necessary for your operating system and compiler.

Plugins can be updated and recompiled without needing to recompile the entire CConArtist application, allowing for easy development and testing of new functionality.

When `cconartist` is running and a plugin is added or updated, it is loaded dynamically to extend the application's capabilities.

## Example: A sensorpacket plugin

The following command compiles a simple `sensorpacket` plugin into a dynamic library named `sensorpacket.dylib`.
This plugin can be used to process incoming sensor data packets from TCP connections that are connected to the
sensor server. Sensor packets are received and written directly to the memory mapped sensor stream.

##
        clang -Wall -O2 -fPIC -dynamiclib -I../include sensorpacket.cpp -o sensorpacket.dylib

## Example: A imagepacket plugin

The following command compiles a simple `imagepacket` plugin into a dynamic library named `imagepacket.dylib`.
This plugin can be used to process incoming image data packets from TCP connections that are connected to the
image server. Image packets are received and written directly to the memory mapped image stream.

##
        clang -Wall -O2 -fPIC -dynamiclib -I../include imagepacket.cpp -o imagepacket.dylib

## Example: A GeekOpen (JSON) plugin

GeekOpen devices (wall panel light switches, etc..) can be configured to send JSON formatted data packets to a TCP server.
JSON messages are received and written directly to the memory mapped GeekOpen JSON stream.
The following command compiles a simple `geekopen` plugin into a dynamic library named `geekopen.dylib`.

##
        clang -Wall -O2 -fPIC -dynamiclib -I../include geekopen.cpp -o geekopen.dylib

## Notes

- Ensure that you have the necessary development tools and libraries installed for compiling dynamic libraries on your system.
- The `-fPIC` flag is used to generate position-independent code, which is required for shared libraries.
- The `-dynamiclib` flag tells the compiler to create a dynamic library
- The filename will be used as the plugin name when loading it in CConArtist and your config can specify for each server
  which plugins to use to decode incoming data.
