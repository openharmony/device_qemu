# Qemu ARM Virt 教程

## 1. 简介

`arm_virt/` 子目录包含部分Qemu ARM虚拟化平台验证的OpenHarmony kernel\_liteos\_a的适配代码，含驱动配置、板端配置等。

ARM 虚拟化平台是一个 `qemu-system-arm` 的目标设备，通过它来模拟一个通用的、基于ARM架构的单板。
Qemu中machine为 **virt** 的单板就是这种可配置的，例如：选择核的类型、核的个数、内存的大小和安全特性等，单板设备的配置。

这次模拟的配置是：Cortex-A7架构，1个CPU，带安全扩展，GICv2，1G内存。
提示: 系统内存硬编码为32MB。

## 2. 环境搭建

参考链接: [环境搭建](https://gitee.com/openharmony/docs/blob/master/zh-cn/device-dev/quick-start/quickstart-lite-env-setup.md)

## 3. 获取源码

参考链接: [代码获取](https://gitee.com/openharmony/docs/blob/master/zh-cn/device-dev/get-code/sourcecode-acquire.md)

## 4. 源码构建

在已经获取的源码根目录，请输入：

```
hb set
```

选择ohemu下的`display_qemu`选项，输出如下：

```
[OHOS INFO] Input code path: .
OHOS Which product do you need?  display_qemu
```

然后执行构建命令如下：

```
hb build
```

这个命令构建会产生 `liteos.bin`、`rootfs_jffs2.img` 和 `userfs_jffs2.img`  的镜像文件。

在构建完成之后，对应的镜像文件在如下目录：

```
out/arm_virt/display_qemu/liteos.bin
out/arm_virt/display_qemu/rootfs_jffs2.img
out/arm_virt/display_qemu/userfs_jffs2.img
```

## 5. 在Qemu中运行镜像

a) 如果没有安装 `qemu-system-arm` ，安装请参考链接 [Qemu installation](https://gitee.com/openharmony/device_qemu/blob/master/README_zh.md)

提示：当前引入的功能在virt-5.1的目标machine已经完成测试，不保证所有的Qemu版本都能够运行成功，因此需要保证你的qemu-system-arm
版本尽可能在5.1及以上。

b) 制作以及运行镜像

在代码根目录下，编译后会生成qemu-run脚本，可直接运行该脚本，根据脚本提示制作、运行镜像。

执行`./qemu-run --help`提示如下：

```
Usage: ./qemu-run [OPTION]...
Make a qemu image(flash.img) for OHOS, and run the image in qemu according
to the options.

    Options:

    -f, --force                rebuild flash.img
    -n, --net-enable           enable net
    -h, --help                 print help info

    By default, flash.img will not be rebuilt if exists, and net will not
    be enabled.
```

默认不加参数的情况下，网络不会自动配置。当根目录镜像文件flash.img存在时，镜像不会被重新制作。

c) 退出qemu环境

按下`Ctrl-A + x`可退出qemu虚拟环境。

## 6. 用法示例

- [向内核传递调试参数](example.md#sectiondebug)

- [用FAT映像传递文件](example.md#sectionfatfs)

- [添加一个Hello World程序](example.md#addhelloworld)

## FAQ:
1. 当网络配置出现问题时，如何排查问题？

   手动配置主机网桥设备。Linux系统参考以下命令：

   ```
   sudo modprobe tun tap
   sudo ip link add br0 type bridge
   sudo ip address add 10.0.2.2/24 dev br0
   sudo ip link set dev br0 up

   # 以下命令执行一次后即可注释掉
   sudo mkdir -p /etc/qemu
   echo 'allow br0' | sudo tee -a /etc/qemu/bridge.conf
   ```

   配置完成后，用ip addr检查应有如下类似显示。当br0不存在或尖括号中为DOWN时，请重新检查配置命令。

   ```
   5: br0: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc noqueue state DOWN group default qlen 1000
       link/ether 2e:52:52:0e:21:44 brd ff:ff:ff:ff:ff:ff
       inet 10.0.2.2/24 scope global br0
          valid_lft forever preferred_lft forever
   ```

   当系统安装有docker等软件时，系统防火墙可能阻止网桥访问。

   `cat /proc/sys/net/bridge/bridge-nf-call-iptables`会显示结果：1

   这时，可用如下命令打开访问许可：

   ```
   echo 0 | sudo tee /proc/sys/net/bridge/bridge-nf-call-iptables
   ```

   提示：系统网络硬编码为10.0.2.0/24，网关10.0.2.2，默认网址10.0.2.15。不同的客户机实例应使用不同的MAC和IP地址(flash映像文件也最好不同)，MAC地址可通过QEMU命令行传递，IP地址可在OHOS命令行中调整，如`ifconfig vn0 inet 10.0.2.30`，或使用其它方法。

2. qemu-run提示`qemu-system-arm`运行出错时，如何排查问题？

   qemu-run脚本中，完整的执行命令及参数解释如下：

   ```
   qemu-system-arm -M virt,gic-version=2,secure=on -cpu cortex-a7 -smp cpus=1 -m 1G \
        -drive if=pflash,file=flash.img,format=raw \
        -netdev bridge,id=net0 \
        -device virtio-net-device,netdev=net0,mac=12:22:33:44:55:66 \
        -device virtio-gpu-device,xres=800,yres=480 \
        -device virtio-mouse-device \
        -vnc :20 \
        -global virtio-mmio.force-legacy=false
   ```

   ```
   -M                           虚拟机类型，ARM virt，GIC 2中断控制器，有安全扩展
   -cpu                         CPU型号
   -smp                         SMP设置，单核
   -m                           虚拟机可使用的内存限制
   -drive if=pflash             CFI闪存盘设置
   -netdev                      [可选]网卡后端设置，桥接类型
   -device virtio-net-device    [可选]网卡设备
   -device virtio-gpu-device    [可选]GPU设备
   -device virtio-mouse-device  [可选]鼠标设备
   -vnc :20                     [可选]远程桌面连接，端口5920
   -global                      QEMU配置参数，不可调整
   ```

   运行时，qemu-run遇到报错如下报错： failed to parse default acl file

   可能是qemu安装方式不同，导致qemu配置文件路径存在一定差异：

   - 使用源码安装默认在/usr/local/qemu/etc/qemu

   - 使用部分Linux发行版安装工具进行安装时，默认在/ect/qemu/目录下

   可根据实际情况，确认具体配置目录，并进行如下修改：

   ```
   echo 'allow br0' | sudo tee -a <配置文件路径>
   ```


3. 1.1.0LTS版本qemu运行无输出?

   LTS的代码存在一个内核启动缺陷，可以参考如下PR尝试解决问题：

   https://gitee.com/openharmony/kernel_liteos_a/pulls/324
