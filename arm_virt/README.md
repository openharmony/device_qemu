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

Refer to HOWTO guide: [Setting up a development environment](https://gitee.com/openharmony/docs/blob/master/en/device-dev/quick-start/environment-setup.md)

#### 3. Code acquisition

Refer to HOWTO guide: [Code acquisition](https://gitee.com/openharmony/docs/blob/master/en/device-dev/get-code/source-code-acquisition.md)
Note: One can use `repo` to fetch code in a straightforward manner.

#### 4. Building from sources

While being in the fetched source tree root directory, please type:
```
hb set -root $PWD
```
After setting on the root direcotory, please change directory to **device/qemu/arm_virt** to build:

```
cd device/qemu/arm_virt
hb build
```

This will build `OHOS_Image.bin` for Qemu ARM virt machine.
Note: "debug" type of build is currently a default type since, as for other supported debug targets, it incorporates Shell app.
      There is no release build available at this time.

After build is finished, the resulting image can be found in:
```
out/qemu_arm_virt_ca7/OHOS_Image.bin
```
#### 5. Running image in Qemu

a) If not installed, please install `qemu-system-arm`
For details, please refer to the HOWTO: [Qemu installation](https://www.qemu.org/download/)

Note: The introduced functionality was tested on virt-5.1 target machine. It is not guaranteed to work with any other version
      so make sure that your qemu-system-arm emulator is up to date.

b) Prepare flash image file. Now its capacity was hard-coded 64M with 3 partitions, 1st 10M-256K for kernel image, 2nd 256K for bootargs, 3rd 54M for rootfs. Linux host can reference following commands:
```
sudo modprobe mtdram total_size=65536 erase_size=256
sudo mtdpart add /dev/mtd0 kernel 0 10223616
sudo mtdpart add /dev/mtd0 bootarg 10223616 262144
sudo mtdpart add /dev/mtd0 root 10485760 56623104
sudo nandwrite -p /dev/mtd1 out/qemu_arm_virt_ca7/OHOS_Image.bin
echo -e "bootargs=root=cfi-flash fstype=jffs2 rootaddr=0xA00000 rootsize=27M\x0" | sudo nandwrite -p /dev/mtd2 -
sudo nandwrite -p /dev/mtd3 out/qemu_arm_virt_ca7/rootfs_jffs2.img
sudo dd if=/dev/mtd0 of=flash.img
sudo chown USERNAME flash.img
sudo rmmod mtdram
```
Note: bootargs only rootsize is adjustable. 3nd partition beyond it mounted at /storage, writable.

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


Note: OHOS build target name `qemu_arm_virt_ca7` is derived from the above mentioned command.

