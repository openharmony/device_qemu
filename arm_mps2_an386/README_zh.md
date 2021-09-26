# Qemu Arm Cortex-m4 mps2-an386 教程

## 1. 简介
`arm_mps2_an386/` 子目录包含部分Qemu arn cortex-m4虚拟化平台验证的OpenHarmony kernel\_liteos\_m的代码，目录名为*arm_mps2_an386*。
Arm Cortex-m4 虚拟化平台是一个 `qemu-system-arm` 的目标设备，通过它来模拟一个通用的、基于arm cortex-m4架构的单板。

这次模拟的配置是：arm cortex-m4架构，1个CPU，16M内存。

提示: 系统内存硬编码为16MB。

## 2. 环境搭建

[环境搭建](https://gitee.com/openharmony/docs/blob/HEAD/zh-cn/device-dev/quick-start/quickstart-lite-env-setup.md)

编译器安装

1.命令安装

提示：命令安装的工具链无 arm-none-eabi-gdb，无法进行gdb调试

```
$ sudo apt install gcc-arm-none-eabi
```

2.安装包安装

提示：如果已经通过命令安装了gcc-arm-none-eabi， 可以通过命令：`$ sudo apt remove gcc-arm-none-eabi` 卸载之后，再进行安装。

下载工具链[安装包](https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/6-2017q2/gcc-arm-none-eabi-6-2017-q2-update-linux.tar.bz2)。

```
$ chmod 777 gcc-arm-none-eabi-6-2017-q2-update-linux.tar.bz2
$ tar -xvf gcc-arm-none-eabi-6-2017-q2-update-linux.tar.bz2 install_path
```

将安装路径添加到环境变量中:

```
$ vim ~/.bashrc
```

在~/.bashrc最末尾加入:

```
$ export PATH=$PATH:install_path/gcc-arm-none-eabi-6-2017-q2-update/bin
```

## 3. 获取源码

[代码获取](https://gitee.com/openharmony/docs/blob/HEAD/zh-cn/device-dev/get-code/sourcecode-acquire.md)

提示: 可以使用 `repo` 命令来获取源码。

## 4. 源码构建

```
$ cd device/qemu/arm_mps2_an386
$ hb build -f
```

这个命令构建会产生 `liteos` 的镜像文件。

在构建完成之后，对应的镜像文件在如下目录：
```
../../../out/arm_mps2_an386/bin/liteos
```
## 5. 在Qemu中运行镜像

a) 如果没有安装 `qemu-system-arm` ，安装请参考链接:[Qemu安装指导](https://gitee.com/openharmony/device_qemu/blob/HEAD/README_zh.md)

b) 运行

```
$ cd device/qemu/arm_mps2_an386
$ ./qemu_run.sh ../../../out/arm_mps2_an386/bin/liteos
```

## 6. gdb调试

```
$ cd device/qemu/arm_mps2_an386
$ vim liteos_m/config.gni
```

将 `board_opt_flags` 中的

```
board_opt_flags = []
```

编译选项修改为:

```
board_opt_flags = [ "-g" ]
```

保存并退出，重新编译:

```
$ hb build -f
```

在一个窗口中输入命令：

```
$ ./qemu_run.sh gdb ../../../out/arm_mps2_an386/unstripped/bin/liteos
```

在另一个窗口中输入命令：

```
$ arm-none-eabi-gdb ../../../out/arm_mps2_an386/unstripped/bin/liteos
(gdb) target remote localhost:1234
(gdb) b main
```

提示: 采用gdb调试时，可执行文件必须选择 `out/arm_mps2_an386/unstripped/bin` 目录下的可执行文件

更多gdb相关的调试可以查阅：[gdb指导手册](https://sourceware.org/gdb/current/onlinedocs/gdb)。
