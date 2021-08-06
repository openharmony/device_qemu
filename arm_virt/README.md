### Qemu ARM Virt HOWTO

#### 1. Brief introduction
`arm/` subdirectory contains part of the OpenHarmony LiteOS demonstration support for Qemu ARM Virtual Platform,
here called *virt*.
ARM Virtual platform is a `qemu-system-arm` machine target that provides emulation
for a generic, ARM-based board. Virt is somehow configurable, for example
user can select core type, number of cores, amount of memory, security features
as well as, to some extent, on-chip device configuration.

Introduced functionality adds support for Cortex-A7 (1 CPU with security extensions),
GICv2, 1024MB memory virtual platform.

Note: System memory size is hard-coded to 32MB.

#### 2. Setting up environment

Refer to HOWTO guide: [Setting up a development environment](https://gitee.com/openharmony/docs/blob/master/en/device-dev/quick-start/quickstart-lite-env-setup.md)

#### 3. Code acquisition

Refer to HOWTO guide: [Code acquisition](https://gitee.com/openharmony/docs/blob/master/en/device-dev/get-code/sourcecode-acquire.md)
Note: One can use `repo` to fetch code in a straightforward manner.

#### 4. Building from sources

While being in the fetched source tree root directory, please type:

```
hb set
```

Please select the `display_qemu` target under ohemu, output is:

```
[OHOS INFO] Input code path: .
OHOS Which product do you need?  display_qemu
```

Then type:

```
hb build
```

This will build `liteos.bin`、`rootfs_jffs2.bin` and `userfs_jffs2.bin` for Qemu ARM virt machine.
Note: "debug" type of build is currently a default type since, as for other supported debug targets, it incorporates Shell app.
      There is no release build available at this time.

After build is finished, the resulting image can be found in:
```
out/arm_virt/display_qemu/liteos.bin
out/arm_virt/display_qemu/rootfs_jffs2.img
out/arm_virt/display_qemu/userfs_jffs2.img
```
#### 5. Running image in Qemu

a) If not installed, please install `qemu-system-arm`
For details, please refer to the HOWTO: [Qemu installation](https://gitee.com/openharmony/device_qemu/blob/master/README.md)

Note: The introduced functionality was tested on virt-5.1 target machine. It is not guaranteed to work with any other version
      so make sure that your qemu-system-arm emulator is up to date.

b) Prepare flash image file. Now its capacity was hard-coded 64M with 4 partitions,
    1st 10M-256K for kernel image,
    2nd 256K for bootargs,
    3rd 10M-32M for rootfs,
    4th 32M-64M for userfs.
Linux host can reference following commands:
```
OUT_DIR="out/arm_virt/display_qemu/"
sudo modprobe mtdram total_size=65536 erase_size=256
sudo mtdpart add /dev/mtd0 kernel 0 10223616
sudo mtdpart add /dev/mtd0 bootargs 10223616 262144
sudo mtdpart add /dev/mtd0 root 10485760 23068672
sudo mtdpart add /dev/mtd0 user 33554432 33554432
sudo nandwrite -p /dev/mtd1 $OUT_DIR/liteos.bin
echo -e "bootargs=root=cfi-flash fstype=jffs2 rootaddr=10M rootsize=22M useraddr=32M usersize=32M\x0" | sudo nandwrite -p /dev/mtd2 -
sudo nandwrite -p /dev/mtd3 $OUT_DIR/rootfs_jffs2.img
sudo nandwrite -p /dev/mtd4 $OUT_DIR/userfs_jffs2.img
sudo dd if=/dev/mtd0 of=flash.img
sudo chown `whoami` flash.img
sudo rmmod mtdram
```

c) Config host net bridge device. In Linux we can do like this:
```
sudo modprobe tun tap
sudo ip link add br0 type bridge
sudo ip address add 10.0.2.2/24 dev br0
sudo ip link set dev br0 up

# comment these if already done
sudo mkdir -p /etc/qemu
echo 'allow br0' | sudo tee -a /etc/qemu/bridge.conf

# comment this if the file doesn't exist
echo 0 | sudo tee /proc/sys/net/bridge/bridge-nf-call-iptables
```
Note: The guest network is hardcoded as 10.0.2.0/24, gateway 10.0.2.2, default ip 10.0.2.15. Different guest instance should use different MAC and IP(better use different flash image). MAC can be assigned through command line. IP can be changed in OHOS command line, e.g. `ifconfig vn0 inet 10.0.2.30`, or whatever method.

d) Run `qemu-system-arm`, enter user-space command line.

```
qemu-system-arm -M virt,gic-version=2,secure -cpu cortex-a7 -smp cpus=1 -nographic -m 1G -drive if=pflash,file=flash.img,format=raw -netdev bridge,id=net0 -device virtio-net-device,netdev=net0,mac=12:22:33:44:55:66 -global virtio-mmio.force-legacy=false
```


```
Explanation for our system configuration:
-M virt,gic-version=2,secure : runs ARM virtual platform with ARM Generic Interrupt Controller version 2 and security extensions enabled
-smp cpus=1                  : defines 1 CPU system
-m 1G                        : defines system memory to be 1024MB. This limitation will be removed in the future but now,
                               more memory will simply not be visible in the system.
```

#### 6. Usage examples

- [passing debug arguments from command line](example.md#sectiondebug)

- [passing files through FAT image](example.md#sectionfatfs)
