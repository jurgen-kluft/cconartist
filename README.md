# ConArtist

An application that can run multiple TCP and UDP servers and shows them in a graphical interface using ImGui.
It will also display incoming messages in real-time and supports a way to write custom packet handlers.

A packet handler has to implement a C interface with two functions, one for streaming data to a memory stream and
another one for building a UI element to show the parsed data in the GUI.

## Packets

Packet handlers are written in C/C++ and compiled as a shared library, you can see an example in 
`source/plugins/sensorpacket.cpp`.
