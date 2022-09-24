# QEMU x86_64 Virt for Standard System Tutorial

## 1. Overview

The `x86_64_virt/linux` directory contains code that has been verified on the QEMU x86_64 Virt platform for adapting to Linux kernel. The code includes the driver and board configurations.

The x86_64 Virt platform is a `qemu-system-x86_64` target device that simulates a general-purpose board running on the x86_64 architecture.
The board whose **machine** is **microvm** in QEMU is configurable. For example, you can select the core type and quantity, memory size, and security extensions when configuring the board.

The system memory is hardcoded to 1024 MB.

## 2. Setting Up the Environment

For details, see [Environment Setup](https://gitee.com/openharmony/docs/blob/HEAD/en/device-dev/quick-start/quickstart-standard.md)

## 3. Obtaining the Source Code

For details, see [Source Code Acquisition](https://gitee.com/openharmony/docs/blob/HEAD/en/device-dev/get-code/sourcecode-acquire.md).

## 4. Building the Source Code

In the root directory of the obtained source code, run the following command:

```
./build.sh --product-name qemu-x86_64-linux-min
```

After this command is executed, the image files for standard system are generated in out/qemu-x86_64-linux/packages/phone/images/ directory.
qemu-x86_64-linux-min means product with minimum set of components.
qemu-x86_64-linux-headless add application framework related components based on qemu-x86_64-linux-min.

## 5. Running an Image in QEMU

a) If `qemu-system-x86_64` has not been installed, install it. For details, see [Qemu Installation](https://gitee.com/openharmony/device_qemu/blob/HEAD/README.md).

Note: The introduced functions have been tested on the target machine of microvim, but are not available for all QEMU versions. Therefore, you must ensure that the qemu-system-x86_64 version is 5.1 or later.


b) Run the images.

After the source code is built, run the `./vendor/ohemu/qemu_x86_64_linux_min/qemu_run.sh` command, the images built in step 4 will be started.

c) Exit QEMU.

Press `Ctrl-A + x` to exit the QEMU virtual environment.

