## 向内核传递参数<a name="sectiondebug"></a>
---

liteos_a提供了定制bootargs的机制，通过qemu-run可以向内核传递参数，格式为：-b arg0=val0,arg1=val1...。当不与-f参数同时使用时，可在现有flash镜像基础上，仅更新镜像中bootargs分区内容。

传递到内核的参数名字和值均为字符串，具体使用方式请见fs/rootfs有关代码。

## 用MMC映像传递文件<a name="sectionfatfs"></a>
---

MMC映像可用于在宿主机和虚拟机之间传递文件。注意：MMC映像同时用于一些系统文件，防止误删除。

1. 在宿主机上挂载

```
sudo modprobe nbd
sudo qemu-nbd --connect=/dev/nbd0 out/smallmmc.img
sudo mount /dev/nbd0p1 some_directory   # 1st partition, total 3 partitions
```

2. 拷贝

常用的cp、mkdir命令。

3. 卸载。

```
sudo umount /mnt
sudo qemu-nbd -d /dev/nbd0
sudo modprobe -r nbd
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

3. 为helloworld创建BUILD.gn文件
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

**提示**：helloworld最后目录结构为
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

**注意**：helloworld 正常编译后会出现在 out/arm_virt/qemu_small_system_demo/bin中，如果没有，请返回检查相关配置文件中的路径和名称是否有误，并尝试重新编译直到出现helloworld

```
提示：编译完成后，代码根目录下会生成qemu-run脚本，直接运行该脚本默认以非root权限运行qemu环境(不含网络配置)。其他参数配置
详见qemu-run --help
```


7. 运行helloworld

helloworld在qemu虚拟机的bin目录下面，进入qemu虚拟机环境后，在bin目录下执行 ./helloworld，会出现如下信息，表示Hello World程序添加成功

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

### 运行另一台虚拟机

2. 为这个虚拟机单独拷贝一份虚拟机映像。
```
cp flash.img flash2.img
cp out/smallmmc.img out/smallmmc1.img
```

3. 从//vendor/ohemu/qemu_small_system_demo/qemu_run.sh中拷贝出qemu命令：
```
sudo `which qemu-system-arm` -M virt, ...
```

修改映像文件名、MMC映像文件名、MAC地址，删除替换脚本变量；为便于观察可增加-nographic参数。

4. 如果ip地址恰好与第1台虚拟机相同，修改ip地址。在OHOS提示符下：
```
ifconfig wlan0 inet 10.0.2.XX
```

5. 观察：启动快结束时，两台虚拟机的日志显示，相互发现了对方，并试图组网。

## Hack图形桌面<a name="desktop"></a>
---

[applications_sample_camera](https://gitee.com/openharmony/applications_sample_camera)仓库提供了一个小型桌面应用示例。虽然qemu虚拟机没有摄录放驱动，但可以利用hisilicon的SDK，来Hack一个图形桌面。

1. 让//device/soc/hisilicon仓库：支持我们的板子

修改文件`common/hal/{media/BUILD.gn,middleware/BUILD.gn}`，找到`if (board_name == "hispark_taurus" || board_name == "aegis_hi3516dv300")`，加上`board_name == "arm_virt"`。

2. 让//foundation/multimedia/utils/lite/仓库：到hisilicon那儿找库

修改文件`BUILD.gn`，找到
```
      "$ohos_board_adapter_dir/media:hardware_media_sdk",
      "$ohos_board_adapter_dir/middleware:middleware_source_sdk",
```
改成
```
      "//device/soc/hisilicon/common/hal/media:hardware_media_sdk",
      "//device/soc/hisilicon/common/hal/middleware:middleware_source_sdk",
```

3. //device/qemu仓库：增加一个拷贝hi3516dv300的mpp库动作

修改文件`arm_virt/liteos_a/BUILD.gn`，增加一个`"mpp:copy_mpp_libs"`依赖，同时在该目录下增加一个指向`../../../soc/hisilicon/hi3516dv300/sdk_liteos/mpp/`的符号链接`mpp`。

**重要**：这部分库使用了git-lfs形式存储，因此要恢复其正常内容：
```
cd device/soc/hisilicon/hi3516dv300/sdk_liteos/mpp/lib/
git lfs checkout *.so
```

4. //vendor/ohemu仓库：增加桌面应用组件

修改文件`qemu_small_system_demo/config.json`，把`"aafwk_lite"`组件的特性值改为`true`，增加如下新组件
```
      {
        "subsystem": "applications",
        "components": [
          { "component": "camera_sample_app", "features":[] },
          { "component": "camera_screensaver_app", "features":[] }
        ]
      },
      {
        "subsystem": "powermgr",
        "components": [
          { "component": "powermgr_lite", "features":[ "enable_screensaver = true" ] }
        ]
      },
```
