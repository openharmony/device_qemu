# Qemu RISCV-64 Virt 标准系统教程

## 简介

`riscv64_virt/linux` 子目录包含部分Qemu RISCV-64虚拟化平台验证的Linux kernel的适配代码，含驱动配置、板端配置等。

RISCV-64 虚拟化平台是一个 `qemu-system-riscv64` 的目标设备，通过它来模拟一个通用的、基于RISCV-64架构的单板。


提示: 系统内存硬编码为2048MB。

## 环境搭建

参考链接: [环境搭建](https://gitee.com/openharmony/docs/blob/HEAD/zh-cn/device-dev/quick-start/quickstart-standard.md)

## 源码构建

在已经获取的源码根目录，请输入：

```
./build.sh --product-name qemu-riscv64-linux-min
```

在构建完成之后，对应的镜像文件在out/qemu-riscv64-linux/packages/phone/images/目录下。
qemu-riscv64-linux-min表示部件最小集合的产品。

## 运行镜像

如果没有安装 `qemu-system-riscv64` ，安装请参考链接 [Qemu installation](https://gitee.com/openharmony/device_qemu/blob/HEAD/README_zh.md)

提示：当前引入的功能在virt-5.1的目标machine已经完成测试，不保证所有的Qemu版本都能够运行成功，因此需要保证你的qemu-system-riscv64
版本尽可能在5.1及以上。

## 退出qemu环境

先按组合键`Ctrl+a` 再单按`x`键，可退出qemu虚拟环境。
