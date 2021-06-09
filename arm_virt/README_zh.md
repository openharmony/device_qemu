# Qemu ARM Virt 教程

## 1. 简介
`arm/` 子目录包含部分Qemu ARM虚拟化平台验证的OpenHarmony kernel\_liteos\_a的代码，目录名为*virt*。
ARM 虚拟化平台是一个 `qemu-system-arm` 的目标设备，通过它来模拟一个通用的、基于ARM架构的单板。
Qemu中machine为 **virt** 的单板就是这种可配置的，例如：选择核的类型、核的个数、内存的大小和安全特性等，单板设备的配置。

这次模拟的配置是：Cortex-A7架构，1个CPU，带安全扩展，GICv2，1G内存。
提示: 系统内存硬编码为32MB。

## 2. 环境搭建

参考链接: [环境搭建](https://gitee.com/openharmony/docs/blob/master/zh-cn/device-dev/quick-start/%E7%8E%AF%E5%A2%83%E6%90%AD%E5%BB%BA.md)

## 3. 获取源码

参考链接: [代码获取](https://gitee.com/openharmony/docs/blob/master/zh-cn/device-dev/get-code/%E6%BA%90%E7%A0%81%E8%8E%B7%E5%8F%96.md)
提示: 可以使用 `repo` 命令来获取源码。

## 4. 源码构建

在已经获取的源码根目录，请输入：

```
hb set -root $PWD
```

完成根目录设置后，在**device/qemu/arm_virt**目录下进行构建：

```
cd device/qemu/arm_virt
hb build
```

这个命令构建会产生 `OHOS_Image.bin` 的镜像文件。
提示："debug" 构建类型是当前的默认类型，因为参考其他构建类型，它包含Shell的App，当前没有release版本。

在构建完成之后，对应的镜像文件在如下目录：
```
out/qemu_arm_virt_ca7/OHOS_Image.bin
```
## 5. 在Qemu中运行镜像

a) 如果没有安装 `qemu-system-arm` ，安装请参考链接 [Qemu installation](https://www.qemu.org/download/)

提示: 当前引入的功能在virt-5.1的目标machine已经测试过了，不能保证所有的Qemu版本都能够运行成功，因此需要保证你的qemu-system-arm版本尽可能的新。

b) 准备flash映像文件。目前系统硬编码flash容量64M，分三个分区：分区一10M-256K用于内核映像，分区二256K用于启动参数，分区三54M用于rootfs。Linux系统可参考如下命令：
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
提示：bootargs中仅rootsize可调整，分区三rootsize以外空间安装在/storage目录，可读可写。

c) 配置主机网桥设备。Linux系统可参考以下命令：
```
sudo modprobe tun tap
sudo ip link add br0 type bridge
sudo ip address add 10.0.2.2/24 dev br0
sudo ip link set dev br0 up

# 以下命令执行一次后即可注释掉
sudo mkdir -p /etc/qemu
echo 'allow br0' | sudo tee -a /etc/qemu/bridge.conf

# 如果这个文件不存在可删除此命令
echo 0 | sudo tee /proc/sys/net/bridge/bridge-nf-call-iptables
```
提示：系统网络硬编码为10.0.2.0/24，网关10.0.2.2，默认网址10.0.2.15。不同的客户机实例应使用不同的MAC和IP地址(flash映像文件也最好不同)，MAC地址可通过QEMU命令行传递，IP地址可在OHOS命令行中调整，如`ifconfig vn0 inet 10.0.2.30`，或使用其它方法。

d) 运行`qemu-system-arm`，进入用户态命令行。

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

提示: OHOS 构建名 `qemu_arm_virt_ca7` 来源于上述提到的命令。

## 6. 用法示例

- [向内核传递调试参数](example.md#sectiondebug)
