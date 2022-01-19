# CAN Bus Bootloader

### About
This project attempts to create a bootloader that has the capability of writing to flash with commands on the CAN bus.

## Adding the cli tool
The cli tool for this package is included as a submodule.
To add it, run `git submodule inti` followed by `git submodule update`

### Building and Running
You will need a cross-compiler for ARM to compile this project. Specifically, `arm-none-eabi-gcc`.

You will also need `cmake` and `make` to build this project.

To compile, run the following commands

    mkdir build
	cd build
	cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake ../
	make

You would then find `.bin` files generated in the `build/` directory. These `.bin` files would then need to be flashed to the corresponding microcontrollers at address `0x08000000` in order to be fully functional.

### Additional Notes

The above instructions assume versions of `cmake >= 3.21.3` and `arm-none-eabi-gcc >= 10.3.1`. Any version below may throw an error. 

If you're on Ubuntu 18.04 or below, you will need to manually install/update these two, since the apt-get repositories go up to cmake 3.10 and arm-non-eabi-gcc 6.3.1. Also, if you're using a docker container to develop without sudo, run the steps below in the docker root shell. This can be accessed via `docker exec -u root -t -i container_name /bin/bash`

For cmake, do the following: 

Original source: https://apt.kitware.com/

1. Purge the old version of cmake using `sudo apt remove cmake`
2. update apt-get stores `sudo apt-get update`and install wget and gpg `sudo apt-get install gpg wget`
3. Get the signing key to pull from the cmake latest release repository by `wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null`
4. Add the repository to the sources.list.d folder to get the new package using `echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ bionic main' | sudo tee /etc/apt/sources.list.d/kitware.list >/dev/null` (switch out "bionic main" for "xenial main" for 16.04, and "focal main" for 20.04)
5. `sudo apt-get update` to download the most recent version of cmake 
6. `sudo apt-get install cmake` to install cmake
7. Check that the following returns version 3.21.3: `cmake --version`


For `arm-none-eabi-gcc`, you can do the following: 

Original source: https://askubuntu.com/questions/1243252/how-to-install-arm-none-eabi-gdb-on-ubuntu-20-04-lts-focal-fossa

1. Purge the old version of arm-none-eabi-gcc using `sudo apt remove gcc-arm-none-eabi`
2. `wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2` to arm-none-eabi-gcc v10.3.1
3. Extract the package to `/usr/share` using `sudo tar xjf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 -C /usr/share/`
4. Run `sudo ln -s /usr/share/gcc-arm-none-eabi-10.3-2021.10-x86_64/bin/* /usr/bin/` to create a symlink on all the arm command executables
5. Check that the following returns 10.3.1: `arm-none-eabi-gcc --version`








