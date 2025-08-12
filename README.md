# robot-util

C program to do various tasks aligned with robot maintenance and utility, such as the connection of
controllers, and updating of other Docker images.

## Building

This project depends on the following libraries:
- libgpiod 1.6.x - **libgpiod >= 2.0 has breaking changes**

```bash
cmake . -B build
cmake --build build -j 8
```
