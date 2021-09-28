# QEMU（C-SKY 教程）

## 1.简介

QEMU可以模拟内核运行在不同的单板，解除对物理开发板的依赖。`SmartL_E802/`子目录包含部分Qemu C-SKY虚拟化平台验证的OpenHarmony kernel\_liteos\_m的代码，通过它来模拟一个基于C-SKY架构的单板。

## 2.环境搭建

   1. 编译工具链安装

      a) 创建`csky_toolchain`文件夹并进入

         ```shell
         $ mkdir csky_toolchain && cd csky_toolchain
         ```

      b) 下载csky-elfabiv2-tools-x86_64-minilibc-20210423.tar.gz 到`csky_toolchain`文件夹并解压，下载地址：https://occ.t-head.cn/community/download?id=3885366095506644992

         ```shell
         $ wget https://occ-oss-prod.oss-cn-hangzhou.aliyuncs.com/resource/1356021/1619529111421/csky-elfabiv2-tools-x86_64-minilibc-20210423.tar.gz
         $ tar -xf csky-elfabiv2-tools-x86_64-minilibc-20210423.tar.gz
         ```

      c) 将csky-elfabiv2编译工具链加入环境变量(将user_toolchain_xxx_path修改为自己的安装路径)：

         ```shell
         $ vim ~/.bashrc
         $ export PATH=$PATH:user_toolchain_xxx_path/csky_toolchain/bin
         $ source ~/.bashrc
         ```

   2. qemu安装

      使用安装指导请参考C-SKY_QEMU_User_Guide.pdf，下载地址：https://occ.t-head.cn/community/download?id=636946310057951232

      a) 创建`csky_qemu`文件夹并进入

         ```shell
         $ mkdir csky_qemu && cd csky_qemu
         ```

      b) 下载csky-qemu-x86_64-Ubuntu-16.04-20210202-1445.tar.gz到`csky_qemu`文件夹下并解压，下载地址：https://occ.t-head.cn/community/download?id=636946310057951232

         ```shell
         $ wget https://occ-oss-prod.oss-cn-hangzhou.aliyuncs.com/resource/1356021/1612269502091/csky-qemu-x86_64-Ubuntu-16.04-20210202-1445.tar.gz
         $ tar -xf csky-qemu-x86_64-Ubuntu-16.04-20210202-1445.tar.gz
         ```

      c) 将qemu加入环境变量(将user_qemu_xxx_path修改为自己的安装路径):

         ```shell
         $ vim ~/.bashrc
         export PATH=$PATH:user_qemu_xxx_path/csky-qemu/bin
         $ source ~/.bashrc
         ```

## 3.获取harmony源码

[代码获取](https://gitee.com/openharmony/docs/blob/master/zh-cn/device-dev/get-code/sourcecode-acquire.md)

提示: 可以使用 `repo` 命令来获取源码。

## 4.源码构建

   1. 执行hb set命令并选择项目`qemu_csky_mini_system_demo`。

   2. 执行hb clean && hb build命令构建产生 `liteos` 的可执行文件。

      ```shell
      $ hb set
      $ hb clean && hb build
      ```

   3. 在构建完成之后，对应的可执行文件在主目录下：

      ```
      out/SmartL_E802/qemu_csky_mini_system_demo/bin/
      ```

   注：当前的liteos为不带符号表的elf文件，带符号表的elf文件路径如下：

      ```
      out/SmartL_E802/qemu_csky_mini_system_demo/unstripped/bin/
      ```

## 5.在Qemu中运行镜像

   1. 启动qemu(不配合GDB)

      ```shell
      qemu-system-cskyv2 -machine smartl -kernel liteos -nographic
      ```

   2. 启动qemu(配合GDB)

      a) 启动GDB服务器，等待连接

         ```shell
         qemu-system-cskyv2 -machine smartl -kernel liteos -nographic -gdb tcp::23333 -S
         ```

      b) 新建终端并使用GDB连接qemu

         ```shell
         $ csky-abiv2-elf-gdb liteos -ex "target remote localhost:23333"
         ```

   注：如果使用GDB调试，建议在运行镜像时使用带符号表的elf文件。
   注：qemu退出方式为：按下ctrl加a键，然后松开再按下x键。
