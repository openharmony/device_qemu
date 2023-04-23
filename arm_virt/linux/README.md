# QEMU Arm Virt for Standard System Tutorial

## 1. Overview

The `arm_virt/linux` directory contains code that has been verified on the QEMU Arm Virt platform for adapting to Linux kernel. The code includes the driver and board configurations.

The Arm Virt platform is a `qemu-system-arm` target device that simulates a general-purpose board running on the Arm architecture.
The board whose **machine** is **virt** in QEMU is configurable. For example, you can select the core type and quantity, memory size, and security extensions when configuring the board.

This tutorial guides you through the configuration of a board based on the Cortex-A7 architecture, with one CPU, extended secure features, Generic Interrupt Controller versions 2 (GICv2), and 1 GB memory.
The system memory is hardcoded to 1024 MB.

## 2. Setting Up the Environment

For details, see [Environment Setup](https://gitee.com/openharmony/docs/blob/master/en/device-dev/quick-start/Readme-EN.md)

## 3. Obtaining the Source Code

For details, see [Source Code Acquisition](https://gitee.com/openharmony/docs/blob/HEAD/en/device-dev/get-code/sourcecode-acquire.md).

## 4. Building the Source Code

In the root directory of the obtained source code, run the following command:

```
./build.sh --product-name qemu-arm-linux-min --ccache --jobs 4
./build.sh --product-name qemu-arm-linux-headless --ccache --jobs 4
```

After this command is executed, the image files for standard system are generated in out/qemu-arm-linux/packages/phone/images/ directory.
qemu-arm-linux-min means product with minimum set of components.
qemu-arm-linux-headless add application framework related components based on qemu-arm-linux-min.

## 5. Running an Image in QEMU

a) If `qemu-system-arm` has not been installed, install it. For details, see [Qemu Installation](https://gitee.com/openharmony/device_qemu/blob/HEAD/README.md).

Note: The introduced functions have been tested on the target machine of virt-5.1, but are not available for all QEMU versions. Therefore, you must ensure that the qemu-system-arm version is 5.1 or later.


b) Run the images.

After the source code is built, run the `./vendor/ohemu/qemu_arm_linux_min/qemu_run.sh` or `./vendor/ohemu/qemu_arm_linux_headless/qemu_run.sh` command, the images built in step 4 will be started.

c) Exit QEMU.

Press `Ctrl-A + x` to exit the QEMU virtual environment.

