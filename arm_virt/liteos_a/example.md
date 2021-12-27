## 向内核传递参数<a name="sectiondebug"></a>
---

liteos_a提供了定制bootargs的机制，通过qemu-run可以向内核传递参数，格式为：-b arg0=val0,arg1=val1...。当不与-f参数同时使用时，可在现有flash镜像基础上，仅更新镜像中bootargs分区内容。

传递到内核的参数名字和值均为字符串，具体使用方式请见fs/rootfs有关代码。

## 用FAT映像传递文件<a name="sectionfatfs"></a>
---

利用arm virt的第二个CFI flash设备，可以加载FAT格式的映像盘。因为FAT映像制作、挂载、存储文件均比较简单，可由此在宿主机和虚拟机间方便地传递文件。

1. 准备FAT映像
```

dd if=/dev/zero of=fat.img bs=64M count=1
sudo losetup /dev/loop0 fat.img
sudo fdisk /dev/loop0    # 磁盘分区选择MBR格式, FAT16或FAT32
sudo losetup -o 1048576 /dev/loop1 /dev/loop0    # 这里用第一个主分区示例
sudo mkfs.vfat /dev/loop1
```

2. 在虚拟机中挂载
```
qemu-system-arm ...(正常运行参数) \
                -drive if=pflash,file=fat.img,format=raw

OHOS # mount /dev/cfiblkp0 some_dir vfat
```

**注意**：新加的drive参数要在原drive参数的后面。

3. 在宿主机中挂载
```
sudo losetup /dev/loop0 fat.img
sudo losetup -o 1048576 /dev/loop1 /dev/loop0
sudo mount /dev/loop1 some_dir
```

4. 缷载
```
sudo umount some_dir

sudo losetup -d /dev/loop1    # 宿主机时
sudo losetup -d /dev/loop0    # 宿主机时
```

## 添加一个HelloWorld程序<a name="addhelloworld"></a>
---
1. 创建helloworld目录
```
helloworld目录结构如下：
applications/sample/helloworld
applications/sample/helloworld/src
```

2. 创建helloworld.c文件
```
在 applications/sample/helloworld/src下创建helloworld.c文件，并添加如下代码：
```
```
#include <stdio.h>

int main(int argc, char **argv)
{
    printf("\n************************************************\n");
    printf("\n\t\tHello OHOS!\n");
    printf("\n************************************************\n\n");

    return 0;
}
```

3. 为helloword创建BUILD.gn文件
```
在 applications/sample/helloworld下添加BUILD.gn文件，并添加如下代码：
```
```
import("//build/lite/config/component/lite_component.gni")
lite_component("hello-OHOS") {
  features = [ ":helloworld" ]
}
executable("helloworld") {
  output_name = "helloworld"
  sources = [ "src/helloworld.c" ]
  include_dirs = []
  defines = []
  cflags_c = []
  ldflags = []
}
```

**提示**：hellworld最后目录结构为
```
applications/sample/helloworld
applications/sample/helloworld/BUILD.gn
applications/sample/helloworld/src
applications/sample/helloworld/src/helloworld.c
```

4. 在build/lite/components中新建配置文件helloworld.json，并添加如下代码：
```
{
  "components": [
    {
      "component": "hello_world_app",
      "description": "Communication related samples.",
      "optional": "true",
      "dirs": [
        "applications/sample/helloworld"
      ],
      "targets": [
        "//applications/sample/helloworld:hello-OHOS"
      ],
      "rom": "",
      "ram": "",
      "output": [],
      "adapted_kernel": [ "liteos_a" ],
      "features": [],
      "deps": {
        "components": [],
        "third_party": []
      }
    }
  ]
}
```

**注意**：helloworld.json中dirs和targets的属性值是不带src的

5. 在vendor/ohemu/qemu_small_system_demo/config.json配置文件中找到subsystems属性，并下面追加helloworld的subsystem配置，配置参考如下：
```
    {
      "subsystem": "helloworld",
       "components": [
        { "component": "hello_world_app", "features":[] }
      ]
    }
```

**注意**：修改JSON配置的时候一定要将多余的逗号去掉，否则编译时会报错

6. 编译并构建qemu虚拟环境

参考链接: [编译方法](README_zh.md)

**注意**：helloworld 正常编译后会出现在 out/arm_virt/qemu_small_system_demo/bin中，如果没有，请返回检查相关配置文件中的路径和名称是否有误，并尝试重新编译直到出现helloword

```
提示：编译完成后，代码根目录下会生成qemu-run脚本，直接运行该脚本默认以非root权限运行qemu环境(不含网络配置)。其他参数配置
详见qemu-run --help
```


7. 运行helloworld

helloworld在qemu虚拟机的bin目录下面，进入qemu虚拟机环境后，在bin目录下执行 ./helloword，会出现如下信息，表示Hello World程序添加成功

```
OHOS # ./helloworld
OHOS #
************************************************

                Hello OHOS!

************************************************
```

## 运行简单图形demo程序<a name="simple_ui_demo"></a>
---

说明：这次操作指导主要是基于noVNC的方式进行vnc链接，用作屏幕显示。

1. 在一个terminal中运行qemu程序

```
./qemu-run
```

2. 在另外一个terminal中运行noVNC程序代理VNC server，以便远端浏览器通过链接地址
访问vnc server，需要知道本机的IP

```
wget https://github.com/novnc/noVNC/archive/refs/tags/v1.2.0.tar.gz
tar -zxvf v1.2.0.tar.gz
cd noVNC-1.2.0/
./utils/launch.sh --vnc localhost:5920
```

后面会显示一个链接，用于vnc访问

```
Navigate to this URL:

    http://ubuntu:6080/vnc.html?host=ubuntu&port=6080
```

其中ubuntu表示域名，需要替换为对应的本机IP，例如本机IP是192.168.66.106，那么访
问的链接地址为

```
http://192.168.66.106:6080/vnc.html?host=192.168.66.106&port=6080
```

3. 在第一个terminal中运行`simple_ui_demo`程序

```
./bin/simple_ui_demo
```

console每秒输出为图形的帧率

```
01-01 00:00:46.990 10 44 D 00000/UiDemo: 53 fps
```

4. 在浏览器中看到的vnc输出即为`960*480`的屏幕显示输出，并且能够用鼠标按钮点击。

5. 退出

在terminal中输入`Ctrl + c`

## 观察dsoftbus组网发现<a name="dsoftbus_discover"></a>
---

### 运行一台虚拟机

1. 启动结束后，观察日志显示dsoftbus试图发现设备。
```
./qemu-run -n
```

2. 修改ip地址。因为系统硬编码了虚拟机的网络参数，为区别两台虚拟机，需要手工调整一下参数。在OHOS提示符下：
```
ifconfig eth0 inet 10.0.2.16
```

### 运行另一台虚拟机

3. 为这个虚拟机单独拷贝一份虚拟机映像。
```
cp flash.img flash2.img
```

4. 从//vendor/ohemu/qemu_small_system_demo/qemu_run.sh中拷贝出如下命令：
```
sudo `which qemu-system-arm` -M virt,gic-version=2,secure=on -cpu cortex-a7 -smp cpus=1 -m 1G -drive \
  if=pflash,file=./flash2.img,format=raw -global virtio-mmio.force-legacy=false -netdev bridge,id=net0 \
  -device virtio-net-device,netdev=net0,mac=12:22:33:44:55:88 \
  -device virtio-gpu-device,xres=800,yres=480 -device virtio-tablet-device \
  -device virtio-rng-device -nographic
```

注意：这里修改了映像文件名、MAC地址；删除了一些脚本变量；为便于观察增加了nographic参数。

5. 观察：启动快结束时，两台虚拟机的日志显示，相互发现了对方，并试图组网。
