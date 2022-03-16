# Qemu ARM Virt 标准系统教程

## 1. 简介

`arm_virt/linux` 子目录包含部分Qemu ARM虚拟化平台验证的Linux kernel的适配代码，含驱动配置、板端配置等。

ARM 虚拟化平台是一个 `qemu-system-arm` 的目标设备，通过它来模拟一个通用的、基于ARM架构的单板。
Qemu中machine为 **virt** 的单板就是这种可配置的，例如：选择核的类型、核的个数、内存的大小和安全特性等，单板设备的配置。

这次模拟的配置是：Cortex-A7架构，1个CPU，带安全扩展，GICv2，1G内存。
提示: 系统内存硬编码为1024MB。

## 2. 环境搭建

参考链接: [环境搭建](https://gitee.com/openharmony/docs/blob/HEAD/zh-cn/device-dev/quick-start/quickstart-standard.md)

## 3. 获取源码

参考链接: [代码获取](https://gitee.com/openharmony/docs/blob/HEAD/zh-cn/device-dev/get-code/sourcecode-acquire.md)

## 4. 源码构建

在已经获取的源码根目录，请输入：

```
./build.sh --product-name qemu-arm-linux-min --ccache --jobs 4
./build.sh --product-name qemu-arm-linux-headless --ccache --jobs 4
```

在构建完成之后，对应的镜像文件在out/qemu-arm-linux/packages/phone/images/目录下。
qemu-arm-linux-min表示部件最小集合的产品。
qemu-arm-linux-headless表示在最小集合基础上，支持无屏幕的用户程序框架部件集合的产品。


## 5. 在Qemu中运行镜像

a) 如果没有安装 `qemu-system-arm` ，安装请参考链接 [Qemu installation](https://gitee.com/openharmony/device_qemu/blob/HEAD/README_zh.md)

提示：当前引入的功能在virt-5.1的目标machine已经完成测试，不保证所有的Qemu版本都能够运行成功，因此需要保证你的qemu-system-arm
版本尽可能在5.1及以上。

b) 运行镜像

执行`./vendor/ohemu/qemu_arm_linux_min/qemu_run.sh`或`./vendor/ohemu/qemu_arm_linux_headless/qemu_run.sh`即可运行步骤4生成的镜像。

c) 退出qemu环境

按下`Ctrl-A + x`可退出qemu虚拟环境。

