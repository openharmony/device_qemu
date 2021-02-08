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

Refer to HOWTO guide: [Setting up a development environment](https://gitee.com/openharmony/docs/blob/master/docs-en/quick-start/setting-up-a-development-environment-1.md)

#### 3. Code acquisition

Refer to HOWTO guide: [Code acquisition](https://gitee.com/openharmony/docs/blob/master/docs-en/get-code/source-code-acquisition.md)  
Note: One can use `repo` to fetch code in a straightforward manner.

#### 4. Building from sources

While being in the fetched source tree root directory, please type:
```
./build.py qemu_arm_virt_ca7 -b debug
```
This will build `OHOS_Image` for Qemu ARM virt machine.  
Note: "debug" type of build is currently a default type since, as for other supported debug targets, it incorporates Shell app.
      There is no release build available at this time.  

After build is finished, the resulting image can be found in:
```
out/qemu_arm_virt_ca7/OHOS_Image
```
#### 5. Running image in Qemu

a) If not installed, please install `qemu-system-arm`  
For details, please refer to the HOWTO: [Qemu installation](https://www.qemu.org/download/)   

Note: The introduced functionality was tested on virt-5.1 target machine. It is not guaranteed to work with any other version
      so make sure that your qemu-system-arm emulator is up to date.

b) Prepare flash image file. Now its capacity was hard-coded 64M with 3 partitions, 1st 10M for kernel image, 2nd 27M for rootfs, 3rd 27M for userfs. Linux host can reference following commands:
```
sudo modprobe mtdram total_size=65536 erase_size=128 writebuf_size=2048
sudo mtdpart add /dev/mtd0 kernel 0 10485760
sudo mtdpart add /dev/mtd0 root 10485760 28311552
sudo mtdpart add /dev/mtd0 user 38797312 28311552
sudo nandwrite -p /dev/mtd1 out/qemu_arm_virt_ca7/OHOS_Image.bin
sudo nandwrite -p /dev/mtd2 out/qemu_arm_virt_ca7/rootfs.img
sudo nandwrite -p /dev/mtd3 out/qemu_arm_virt_ca7/userfs.img
sudo dd if=/dev/mtd0 of=flash.img
sudo chown USERNAME flash.img
```

c) Run `qemu-system-arm`, enter user-space command line.

```
qemu-system-arm -M virt,gic-version=2,secure -cpu cortex-a7 -smp cpus=1 -nographic -m 1G -drive if=pflash,file=flash.img,format=raw
```


```
Explanation for our system configuration:  
-M virt,gic-version=2,secure : runs ARM virtual platform with ARM Generic Interrupt Controller version 2 and security extensions enabled
-smp cpus=1                  : defines 1 CPU system
-m 1G                        : defines system memory to be 1024MB. This limitation will be removed in the future but now,
                               more memory will simply not be visible in the system.
```


Note: OHOS build target name `qemu_arm_virt_ca7` is derived from the above mentioned command.

