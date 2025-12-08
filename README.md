# ConArtist

ConArtist consists of four main components: the library, the server, the GUI, and the packet handlers.

## 1. Library

The core library provides functionality for managing data streams, handling connections, and processing packets.
This libary is shared between the server application and the GUI application. The core library code is located in the
`source/main/cpp/` directory.

## 2. Server

The server is responsible for handling incoming TCP and UDP connections. It can run multiple servers simultaneously, and 
it manages the connections and data streams. The server code is located in the `source/main/cpp/conman.cpp` file.

The server mainly outputs data streams to memory-mapped files on disk, which can then be read by the GUI application.

## 3. GUI

The GUI is a separate application that opens the data streams (read-only) created by the server and displays the incoming
messages in real-time. It uses ImGui for rendering the interface and is located in the `source/gui/cpp/main.cpp` file.

## 4. Packets

Packet handlers are written in C/C++ and compiled as a shared library, you can see an example in 
`source/plugins/sensorpacket.cpp`.
