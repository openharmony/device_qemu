# QEMU Arm Virt Tutorial - liteos_a

## 1. Overview

The `arm_virt/` directory contains code that has been verified on the QEMU Arm Virt platform for adapting to OpenHarmony kernel\_liteos\_a. The code includes the driver and board configurations.

The Arm Virt platform is a `qemu-system-arm` target device that simulates a general-purpose board running on the Arm architecture.
The board whose **machine** is **virt** in QEMU is configurable. For example, you can select the core type and quantity, memory size, and security extensions when configuring the board.

This tutorial guides you through the configuration of a board based on the Cortex-A7 architecture, with one CPU, extended secure features, Generic Interrupt Controller versions 2 (GICv2), and 1 GB memory.
The system memory is hardcoded to 64 MB.

## 2. Setting Up the Environment

For details, see [Environment Setup](https://gitee.com/openharmony/docs/blob/HEAD/en/device-dev/quick-start/quickstart-lite-env-setup.md).

## 3. Obtaining the Source Code

For details, see [Source Code Acquisition](https://gitee.com/openharmony/docs/blob/HEAD/en/device-dev/get-code/sourcecode-acquire.md).

## 4. Building the Source Code

In the root directory of the obtained source code, run the following command:

```
hb set
```

Select `qemu_small_system_demo` under **ohemu**.

Run the following build command:

```
hb build
```

After this command is executed, the image files `OHOS_Image.bin`, `rootfs_jffs2.img`, and `userfs_jffs2.img` are generated in out/arm_virt/qemu_small_system_demo/ directory.

## 5. Running an Image in QEMU

a) If `qemu-system-arm` has not been installed, install it. For details, see [Qemu Installation](https://gitee.com/openharmony/device_qemu/blob/HEAD/README.md).

Note: The introduced functions have been tested on the target machine of virt-5.1, but are not available for all QEMU versions. Therefore, you must ensure that the qemu-system-arm version is 5.1 or later.

b) Create and run an image.

After the source code is built, the **qemu-run** script is generated in the root directory of the code. You can run the script to create and run an image as prompted.

Run the `./qemu-run --help` command. The following information is displayed:

```
Usage: ./qemu-run [OPTION]...
Make a qemu image(flash.img) for OHOS, and run the image in qemu according
to the options.

    Options:

    -f, --force                rebuild flash.img
    -n, --net-enable           enable net
    -l, --local-desktop        no VNC
    -b, --bootargs             additional boot arguments(-bk1=v1,k2=v2...)
    -g, --gdb                  enable gdb for kernel
    -h, --help                 print help info

    By default, flash.img will not be rebuilt if exists, and net will not
    be enabled, gpu enabled and waiting for VNC connection at port 5920.
```

Simulated network interface is wireless card wlan0, but has no real wifi functions. By default, the network will not be automatically configured if no parameter is specified. If the root directory image file **flash.img** exists, the image will not be re-created.

Note: On the first run, script will also create a MMC image mainly for system and user data files. The 1st partition will be mounted at /sdcard, 2nd at /userdata, and 3rd is reserved. It is stored at OHOS source tree 'out' sub-directory, named 'smallmmc.img'. Whenever it exists, script will never try to touch it again. More information please refer to `vendor/ohemu/qemu_small_system_demo/qemu_run.sh`.

c) Exit QEMU.

Press `Ctrl-A + x` to exit the QEMU virtual environment.

## 6. gdb debug

Install `gdb-multiarch` package:
```
sudo apt install gdb-multiarch
```
Then,
```
cd ohos/vendor/ohemu/qemu_small_system_demo/kernel_configs
vim debug.config
```

Modify `LOSCFG_CC_STACKPROTECTOR_ALL=y` to:

```
# LOSCFG_CC_STACKPROTECTOR_ALL is not set
LOSCFG_COMPILE_DEBUG=y
```

Save and exit, recompile under OHOS root directory:

```
hb build -f
```

In a window to enter the command:

```
./qemu-run -g
```

In another window to enter the command:

```
gdb-multiarch out/arm_virt/qemu_small_system_demo/OHOS_Image
(gdb) target remote localhost:1234
```

More GDB related debugging can refer to [GDB instruction manual](https://sourceware.org/gdb/current/onlinedocs/gdb).

## 7. Example

- [Transferring Parameters to the Kernel](example.md#sectiondebug)

- [Transferring Files Using MMC Images](example.md#sectionfatfs)

- [Adding a Hello World Program](example.md#addhelloworld)

- [Running the Graphic Demo](example.md#simple_ui_demo)

- [Observe dsoftbus Discovery](example.md#dsoftbus_discover)

- [Hack A Sample Desktop](example.md#desktop)

## FAQ:
1. How do I locate a network configuration problem?

   Manually configure a host network bridge. For Linux, run the following commands:

   ```
   sudo modprobe tun tap
   sudo ip link add br0 type bridge
   sudo ip address add 10.0.2.2/24 dev br0
   sudo ip link set dev br0 up

   # The following commands can be commented out after being executed:
   sudo mkdir -p /etc/qemu
   echo 'allow br0' | sudo tee -a /etc/qemu/bridge.conf
   ```

   Run the **ip addr** command to check the configuration. If **br0** does not exist or the value in the angle brackets is **DOWN**, check the configuration commands again.

   ```
   5: br0: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc noqueue state DOWN group default qlen 1000
       link/ether 2e:52:52:0e:21:44 brd ff:ff:ff:ff:ff:ff
       inet 10.0.2.2/24 scope global br0
          valid_lft forever preferred_lft forever
   ```

   If software such as Docker has been installed in the system, the system firewall may prevent the bridge from accessing the system. Run the following command:

   `cat /proc/sys/net/bridge/bridge-nf-call-iptables`

   **1** is displayed. Run the following command to allow the access from the network bridge:

   ```
   echo 0 | sudo tee /proc/sys/net/bridge/bridge-nf-call-iptables
   ```

   Note: The system is hardcoded to **10.0.2.0/24** for the sub-network, **10.0.2.2** for the gateway, and random IP address. Use different MAC addresses, IP addresses, and flash, MMC images for different client instances. The MAC address can be transferred using the QEMU command line. The IP address can be adjusted in the OHOS command line, for example, using `ifconfig wlan0 inet 10.0.2.30` or other methods.

2. How do I troubleshoot the error when running `qemu-system-arm`?

   The commands and parameters in the **qemu-run** script are as follows:

   ```
   qemu-system-arm -M virt,gic-version=2,secure=on -cpu cortex-a7 -smp cpus=1 -m 1G \
        -drive if=pflash,file=flash.img,format=raw \
        -drive if=none,file=./out/smallmmc.img,format=qcow2,id=mmc
        -device virtio-blk-device,drive=mmc \
        -netdev bridge,id=net0 \
        -device virtio-net-device,netdev=net0,mac=12:22:33:44:55:66 \
        -device virtio-gpu-device,xres=960,yres=480 \
        -device virtio-tablet-device \
        -device virtio-rng-device \
        -vnc :20 \
        -s -S \
        -global virtio-mmio.force-legacy=false
   ```

   ```
   -M                           Virtual machine type, ARM virt, GICv2, and extended security features
   -cpu                         CPU model
   -smp                         SMP setting, single core
   -m                           Maximum memory size that can be used by the virtual machines
   -drive if=pflash             CFI flash drive setting
   -drive if=none               Block device image setting
   -netdev                      [optional] NIC bridge type
   -device virtio-blk-device    Block device
   -device virtio-net-device    [optional] NIC device
   -device virtio-gpu-device    GPU device
   -device virtio-tablet-device Input device
   -device virtio-rng-device    Random generator device
   -vnc: 20                     [recommend] Remote desktop connection, port 5920
   -s -S                        [optional] gdb single step debug
   -global                      QEMU configuration parameter, which cannot be changed
   ```

   If the error message "failed to parse default acl file" is displayed when **qemu-run** is executed:

   The error may be caused by the QEMU configuration file path, which varies with the QEMU installation mode. The default QEMU configuration file path is:

   - **/usr/local/qemu/etc/qemu** if QEMU is installed from the source code.

   - **/ect/qemu/** if QEMU is installed by using a Linux distribution installation tool.

   Determine the configuration file path and run the following command:

   ```
   echo 'allow br0' | sudo tee -a <Configuration file path>
   ```


3. What do I do if there is no output after QEMU of LTS 1.1.0 is executed?

   The LTS code has a kernel startup defect. You can try to resolve the problem by referring to the following:

   https://gitee.com/openharmony/kernel_liteos_a/pulls/324


4. VNC window has no cursor?

   Virtio-tablet is an emulated tablet input device. Neither QEMU captures nor the virtual machine displays it. The cursor is displayed by VNC client. Please check VNC client options.