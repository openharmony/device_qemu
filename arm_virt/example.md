## 向内核传递调试参数<a name="sectiondebug"></a>
---

### QEMU命令行参数：

-fw_cfg name=opt/d,string="arg0=071 arg1='abcf arg2=0x81 arg3=81"

双引号部分即为要向内核传递的参数：

- 等号左侧为参数名字，可以自由选择
- 等号右侧为值，支持两种类型
    - 数值，包括0开头的八进制，0x或0X开头的十六进制，其它为十进制，可以有符号，数值最大为long long
    - 字符串，为与数值区分，须以单引号开始（传递到内核前丢弃）
- 参数间以空格分隔
- 参数总长度不超过99个字节

命令行中其它部分为固定值。

**注意**：这个简单驱动没有太多的容错能力，并且QEMU命令行也有一些要求，因此对字符串类型值尽量不要出现逗号、等号、空格等，数值应适合所提供的存储区域；如果需要，可参阅QEMU fw_cfg文档关于使用文件传参的描述。

### 示例

1. 假设内核代码有两个参数需要测试
```
    int queueSize;
    char fileName[MAX_NAME_LENGTH];
```

2. 在初始处增加获取命令行参数的代码
```
#ifdef LOSCFG_PLATFORM_QEMU_ARM_VIRT_CA7
    extern void GetDebugArgs(const char *argName, void *valBuf, unsigned valLen);
    GetDebugArgs("qsz", &queueSize, sizeof(int));
    GetDebugArgs("file", fileName, MAX_NAME_LENGTH);
#endif
```

3. 通过QEMU命令行向内核传递这两个参数值
```
qemu-system-arm ...(正常运行参数) \
                -fw_cfg name=opt/d,string="qsz=0x400 file='test.txt"
```

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

5. 在vendor/ohemu/display_qemu_liteos_a/config.json配置文件中找到subsystems属性，并下面追加helloworld的subsystem配置，配置参考如下：
```
    {
      "subsystem": "helloworld",
       "components": [
        { "component": "hello_world_app", "features":[] }
      ]
    }
```

**注意**：vendor/ohemu/display_qemu_liteos_a/config.json对应的编译模板为 display_qemu，后面编译时需要选择这个模板；另外修改JSON配置的时候一定要将多余的逗号去掉，否则编译时会报错

6. 编译并构建qemu虚拟环境

参考链接: [编译方法](README_zh.md) 

**注意**：helloworld 正常编译后会出现在 out/arm_virt/display_qemu/bin中，如果没有，请返回检查相关配置文件中的路径和名称是否有误，并尝试重新编译直到出现helloword

```
提示: qemu-init和qemu-run两个文件已经包含qemu相关的操作指令，可以先执行./qemu-init 然后再执行./qemu-run
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