# LibUv

What if the cconartist server only listens on a Unix socket and accepts connection that send it sensor data.
This would mean that we can remove Libuv, which also means way less surface area to deal with in C++.
The goal here is to have this storage server be as light weight as possible with as little points of failure
as possible.

Then we write a small Golang application that can accept MQTT, TCP and UDP connections, analyzing the messages and
pipes them to the Unix socket as sensor data. It can at the same time also pipe them through to a Unix socket for
an the Automation application. We could have many other small applications, C++ or Golang, that connect through
Unix sockets to send/receive Sensor data, for example apps that are HomeKit compatible (virtual) switches.
