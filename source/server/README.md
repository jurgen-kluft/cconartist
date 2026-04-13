# Server

The server is only responsible for handling incoming sensor packets and directly writing them to the associated memory mapped file.

## Configuration

Currently there are 3 open ports that the server listens to:

- SENSOR TCP: 31337
- SENSOR UDP: 31338
- IMAGE TCP: 31340

## Memory Mapped Files

For each unique combination of MacAddress and SensorType, the server creates a memory-mapped file in the `/streams/` directory. The file is named in the format `SensorType/MacAddress.mmap`. For example:

- `/streams/temperature/AABBCCDDEEFF.mmap`
- `/streams/humidity/AABBCCDDEEFF.mmap`
- `/streams/pressure/AABBCCDDEEFF.mmap`

