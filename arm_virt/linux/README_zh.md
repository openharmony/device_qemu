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

### 5.1 qemu_run.sh使用说明

```C
Usage: qemu-run [OPTION]...
Run a OHOS image in qemu according to the options.
    -e,  --exec image_path    images path, including: zImage-dtb, ramdisk.img, system.img, vendor.img, userdata.img
    -g,  --gdb                enable gdb for kernel.
    -n,  --network            auto setup network for qemu (sudo required).
    -i,  --instance id        start qemu images with specified instance id (from 01 to 99).
                              it will also setup network when running in multiple instance mode.
         -f                   force override instance image with a new copy.
    -h,  --help               print this help info.

    If no image_path specified, it will find OHOS image in current working directory; then try .

    When setting up network, it will create br0 on the host PC with the following information:
        IP address: 192.168.100.1
        netmask: 255.255.255.0

    The default qemu device MAC address is [00:22:33:44:55:66], default serial number is [0023456789].
    When running in multiple instances mode, the MAC address and serial number will increase with specified instance ID as follow:
        MAC address:    {instanceID}:22:33:44:55:66
        Serial number:  {instanceID}23456789
```

qemu_run.sh默认会启动当前工作目录或out/qemu-arm-linux/packages/phone/images目录下的系统镜像。

默认的qemu虚拟机没有网络连接。如果需要使能网络连接，可以添加-n选项。此选项会完成以下几个事物：

- 主机侧创建虚拟网桥

  主机侧会创建br0的网桥，用于与qemu ram虚拟机设备进行网络通信。

  br0默认IP地址为192.168.100.1，子网掩码为255.255.255.0。

  由于主机侧创建虚拟网桥需要管理员权限，因此，需要通过sudo命令执行qemu_run.sh。


- 虚拟机侧创建虚拟网卡

  启动qemu arm虚拟机时会为虚拟机设备创建一个网卡，该网卡的默认MAC地址是12:22:33:44:55:66，在虚拟机的网络接口名称是eth0。

  虚拟机启动后，可以通过ifconfig eth0 192.168.100.2给虚拟机设置上IP地址（其它IP地址都可以，只要在同一个网段内）。

  通过此设置，主机就可以和虚拟机进行网络通信。可以使用hdc与虚拟机交互。

正常的OHOS设备都有一个唯一的设备序列号，该序列号会用于各个业务进行设备唯一标识。使用qemu虚拟机启动时，默认的序列号是0123456789。

### 5.2 qemu_run.sh多实例运行

有时需要运行多个qemu arm虚拟机实例，用于验证分布式组网场景。此时，可以在调用qemu_run.sh时传入-i指定不同实例号。每个不同实例号运行多虚拟机设备都有不同的MAC地址和SN号，以此来模拟多个虚拟设备。MAC地址和SN号的分配规则如下：

        MAC address:    {instanceID}:22:33:44:55:66
        Serial number:  {instanceID}23456789

instanceID的取值格式为两个数字字符：范围为01到99。

当镜像输出目录中已存在该instanceID的实例时，默认不重新拷贝镜像。若要重新拷贝镜像可在调用qemu_run.sh时传入-f选项将原镜像覆盖。
