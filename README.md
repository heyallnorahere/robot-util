# robot-util

C program to do various tasks aligned with robot maintenance and utility, such as the connection of
controllers, and updating of other Docker images.

Of course, this only builds on Linux.

## Building

This project depends on the following libraries:
- libcjson
- libcurl
- libgpiod 1.6.x - **libgpiod >= 2.0 has breaking changes**
- libbluetooth
- libglib2.0

`pkg-config` is also required for configuration.

```bash
cmake . -B build
cmake --build build -j 8
```
