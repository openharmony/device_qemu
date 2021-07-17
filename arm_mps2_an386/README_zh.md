# Qemu Arm Cortex-m4 mps2-an386 教程

## 1. 简介
`arm_mps2_an386/` 子目录包含部分Qemu arn cortex-m4虚拟化平台验证的OpenHarmony kernel\_liteos\_m的代码，目录名为*arm_mps2_an386*。
Arm Cortex-m4 虚拟化平台是一个 `qemu-system-arm` 的目标设备，通过它来模拟一个通用的、基于arm cortex-m4架构的单板。

这次模拟的配置是：arm cortex-m4架构，1个CPU，16M内存。

提示: 系统内存硬编码为16MB。

## 2. 环境搭建

[环境搭建](https://gitee.com/openharmony/docs/blob/master/zh-cn/device-dev/quick-start/%E7%8E%AF%E5%A2%83%E6%90%AD%E5%BB%BA.md)

编译器安装

```
sudo apt install gcc-arm-none-eabi
```

## 3. 获取源码

[代码获取](https://gitee.com/openharmony/docs/blob/master/zh-cn/device-dev/get-code/%E6%BA%90%E7%A0%81%E8%8E%B7%E5%8F%96.md)

提示: 可以使用 `repo` 命令来获取源码。

## 4. 源码构建

```
cd device/qemu/arm_mps2_an386
hb build -f
```

这个命令构建会产生 `liteos` 的镜像文件。

在构建完成之后，对应的镜像文件在如下目录：
```
../../../out/arm_mps2_an386/bin/liteos
```
## 5. 在Qemu中运行镜像

a) 如果没有安装 `qemu-system-arm` ，安装请参考链接:[Qemu安装指导](https://gitee.com/openharmony/device_qemu/blob/master/README_zh.md)

b) 运行

```
cd device/qemu/arm_mps2_an386
./qemu_run.sh ../../../out/arm_mps2_an386/bin/liteos
```
