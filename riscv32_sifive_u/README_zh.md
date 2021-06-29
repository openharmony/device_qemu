# Qemu RISC-V sifive_u 教程

## 1. 简介
`risc-v/` 子目录包含部分Qemu RISC-V虚拟化平台验证的OpenHarmony kernel\_liteos\_m的代码，目录名为*sifive\_u*。
RISC-V 虚拟化平台是一个 `qemu-system-riscv32` 的目标设备，通过它来模拟一个通用的、基于RISC-V架构的单板。

这次模拟的配置是：RISC-V架构，1个CPU，128M内存。

提示: 系统内存硬编码为128MB。

## 2. 环境搭建

参考链接: [环境搭建](https://gitee.com/openharmony/docs/blob/master/zh-cn/device-dev/quick-start/%E7%8E%AF%E5%A2%83%E6%90%AD%E5%BB%BA.md)

## 3. 获取源码

参考链接: [代码获取](https://gitee.com/openharmony/docs/blob/master/zh-cn/device-dev/get-code/%E6%BA%90%E7%A0%81%E8%8E%B7%E5%8F%96.md)

提示: 可以使用 `repo` 命令来获取源码。

## 4. 源码构建

```
cd device/qemu/riscv32_sifive_u
make clean;make -j16
```

这个命令构建会产生 `OHOS_Image` 的镜像文件。

在构建完成之后，对应的镜像文件在如下目录：
```
out/OHOS_Image
```
## 5. 在Qemu中运行镜像

a) 如果没有安装 `qemu-system-riscv32` ，安装请参考链接 [Qemu installation](https://www.qemu.org/download/)

提示: 当前引入的功能在基于qemu 4.0.90版本的目标machine已经测试过了，不能保证所有的Qemu版本都能够运行成功，因此需要保证你的qemu-system-riscv32的版本尽可能为4.0.90。

b) 运行

```
cd device/qemu/riscv32_sifive_u
./qemu-system-riscv32 out/OHOS_Image
```