# CAN Bus Bootloader

### About
This project attempts to create a bootloader that has the capability of writing to flash with commands on the CAN bus.

### Building and Running
You will need a cross-compiler for ARM to compile this project. Specifically, `arm-none-eabi-gcc`.

You will also need `cmake` and `make` to build this project.

To compile, run the following commands

    mkdir build
	cd build
	cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake ../
	make

You would then find `.bin` files generated in the `build/` directory. These `.bin` files would then need to be flashed to the corresponding microcontrollers at address `0x08000000` in order to be fully functional.
