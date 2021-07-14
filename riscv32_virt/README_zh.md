# Qemu RISC-V virt 教程

## 1. 简介
`riscv32_virt/` 子目录包含部分Qemu RISC-V虚拟化平台验证的OpenHarmony kernel\_liteos\_m的代码，目录名为*virt*。
RISC-V 虚拟化平台是一个 `qemu-system-riscv32` 的目标设备，通过它来模拟一个通用的、基于RISC-V架构的单板。

这次模拟的配置是：RISC-V架构，1个CPU，128M内存。

提示: 系统内存硬编码为128MB。

## 2. 环境搭建

[环境搭建](https://gitee.com/openharmony/docs/blob/master/zh-cn/device-dev/quick-start/%E7%8E%AF%E5%A2%83%E6%90%AD%E5%BB%BA.md)

[编译器安装: gcc_riscv32](https://gitee.com/openharmony/docs/blob/master/zh-cn/device-dev/quick-start/%E5%AE%89%E8%A3%85%E5%BC%80%E5%8F%91%E6%9D%BF%E7%8E%AF%E5%A2%83.md), 
提示: [可直接下载](https://repo.huaweicloud.com/harmonyos/compiler/gcc_riscv32/7.3.0/linux/gcc_riscv32-linux-7.3.0.tar.gz)

## 3. 获取源码

[代码获取](https://gitee.com/openharmony/docs/blob/master/zh-cn/device-dev/get-code/%E6%BA%90%E7%A0%81%E8%8E%B7%E5%8F%96.md)

提示: 可以使用 `repo` 命令来获取源码。

## 4. 源码构建

```
cd device/qemu/riscv32_virt
make clean;make -j16
```

这个命令构建会产生 `OHOS_Image` 的镜像文件。

在构建完成之后，对应的镜像文件在如下目录：
```
out/OHOS_Image
```
## 5. 在Qemu中运行镜像

a) 如果没有安装 `qemu-system-riscv32` ，安装请参考链接:[Qemu安装指导](https://gitee.com/openharmony/device_qemu/blob/master/README_zh.md)

b) 运行

(1). qemu 版本 < 5.0.0 

```
cd device/qemu/riscv32_virt
qemu-system-riscv32 -machine virt -m 128M -kernel out/OHOS_Image -nographic -append "root=dev/vda or console=ttyS0"
```

(2). qemu 版本 >= 5.0.0 

```
cd device/qemu/riscv32_virt
./qemu-system-riscv32 out/OHOS_Image
或
qemu-system-riscv32 -machine virt -m 128M -bios none -kernel out/OHOS_Image -nographic -append "root=dev/vda or console=ttyS0"
```
