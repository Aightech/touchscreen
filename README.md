# touchscreen

Simple library to ..............

# Building source code

To build the project run:
```bash
cd touchscreen
mkdir build && cd build
cmake .. && make
```

# Demonstration app

When the project have been built, you can run:
```bash
./touchscreen -h
```
to get the demonstration app usage.

# Example
Open the ![main.cpp](cpp:src/main.cpp) file to get an example how to use the lib.

# Disable touchscreen in X11
```bash
To disable the touchscreen in X11, you can run:
```bash
sudo xinput disable "HID 03eb:214e"
```