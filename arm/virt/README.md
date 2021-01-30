# Qemu ARM Virt 教程

## 1. 简介
`arm/` 子目录包含部分Qemu ARM虚拟化平台验证的OpenHarmony kernel\_liteos\_a的代码，目录名为*virt*。
ARM 虚拟化平台是一个 `qemu-system-arm` 的目标设备，通过它来模拟一个通用的、基于ARM架构的单板。
Qemu中machine为 **virt** 的单板就是这种可配置的，例如：选择核的类型、核的个数、内存的大小和安全特性等，单板设备的配置。

这次模拟的配置是：Cortex-A7架构，1个CPU，带安全扩展，GICv2，1G内存。
提示: 系统内存硬编码为32MB。

## 2. 环境搭建

参考链接: [环境搭建](https://gitee.com/openharmony/docs/blob/master/quick-start/%E6%90%AD%E5%BB%BA%E7%8E%AF%E5%A2%83.md)

## 3. 获取源码

参考链接: [代码获取](https://gitee.com/openharmony/docs/blob/master/get-code/%E6%BA%90%E7%A0%81%E8%8E%B7%E5%8F%96.md)
提示: 可以使用 `repo` 命令来获取源码。

## 4. 源码构建

在已经获取的源码根目录，请输入：

```
./build.py qemu_arm_virt_ca7 -b debug
```

这个命令构建会产生 `OHOS_Image` 的镜像文件。
提示："debug" 构建类型是当前的默认类型，因为参考其他构建类型，它包含Shell的App，当前没有release版本。

在构建完成之后，对应的镜像文件在如下目录：
```
out/qemu_arm_virt_ca7/OHOS_Image
```
## 5. 在Qemu中运行镜像

a) 如果没有安装 `qemu-system-arm` ，安装请参考链接 [Qemu installation](https://www.qemu.org/download/)

提示: 当前引入的功能在virt-5.1的目标machine已经测试过了，不能保证所有的Qemu版本都能够运行成功，因此需要保证你的qemu-system-arm版本尽可能的新。

b) 运行`qemu-system-arm`

```
qemu-system-arm -M virt,gic-version=2,secure -cpu cortex-a7 -smp cpus=1 -nographic -kernel ./out/qemu_arm_virt_ca7/OHOS_Image -m 1G
```

```
Explanation for our system configuration:
-M virt,gic-version=2,secure : runs ARM virtual platform with ARM Generic Interrupt Controller version 2 and security extensions enabled
-smp cpus=1                  : defines 1 CPU system
-m 1G                        : defines system memory to be 1024MB. This limitation will be removed in the future but now,
                               more memory will simply not be visible in the system.
```

提示: OHOS 构建名 `qemu_arm_virt_ca7` 来源于上述提到的命令。

