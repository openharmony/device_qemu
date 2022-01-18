# QEMU（ESP32 教程）

## 1.简介

QEMU可以模拟内核运行在不同的单板，解除对物理开发板的依赖。esp32子目录包含部分Qemu esp32虚拟化平台验证的OpenHarmony kernel\_liteos\_m的代码，它可以用来模拟单核esp32单板。

## 2.环境搭建

   1. esp-idf安装

      使用安装指导请参考：(https://docs.espressif.com/projects/esp-idf/zh_CN/release-v4.1/get-started/index.html)

   2. esptool安装

      a) 步骤1中export.sh脚本会设置esptool工具路径,需要确保esptool工具版本为3.1及以上。
         ```shell
         esptool.py version
         ```
      b) 如果esp-idf自带esptool工具版本过低，需删除当前esptool路径对应的环境变量，并执行以下命令。（python版本建议为3.8及以上）
         ```shell
         python -m pip install esptool
         ```

   3. qemu-system-xtensa编译

      a) 编译安装

         ```shell
         $ git clone https://github.com/espressif/qemu.git
         $ cd qemu
         $ ./configure --target-list=xtensa-softmmu \
            --enable-gcrypt \
            --enable-debug --enable-sanitizers \
            --disable-strip --disable-user \
            --disable-capstone --disable-vnc \
            --disable-sdl --disable-gtk
         ```

      b) 等待编译结束，执行安装命令（如果编译失败请参考https://github.com/espressif/qemu/issues/21）:

         ```shell
         $ ninja -C build
         ```

      c) 将qemu添加到环境变量中(user_qemu_xxx_path修改为自己的安装路径):

         ```shell
         $ vim ~/.bashrc
         $ export QEMU=user_qemu_xxx_path/qemu/build
         $ source ~/.bashrc
         ```

      d) 安装依赖

         ```shell
         $ ldd $QEMU/qemu-system-xtensa
         ```

         根据ldd执行结果，安装缺少的依赖库

         (注：更多安装指导，请参考链接：[Home · espressif/qemu Wiki · GitHub](https://github.com/espressif/qemu/wiki#configure))

## 3.获取源码

[代码获取](https://gitee.com/openharmony/docs/blob/master/zh-cn/device-dev/get-code/sourcecode-acquire.md)

提示: 可以使用 `repo` 命令来获取源码。

## 4.源码构建

   1. 执行hb set命令并选择项目`qemu_xtensa_mini_system_demo`。

   2. 执行hb clean && hb build命令构建产生 `OHOS_Image` 的可执行文件。

      ```shell
      $ hb set
      $ hb clean && hb build
      ```

   3. 在构建完成之后，对应的可执行文件在主目录下：

      ```
      out/esp32/qemu_xtensa_mini_system_demo/
      ```

## 5.在Qemu中运行镜像

   1. 运行qemu(不配合GDB)

      ```shell
      $ ./qemu-run
      ```

   2. 启动qemu(配合GDB)

      a) 启动GDB服务器，等待连接

         ```shell
         $ ./qemu-run -g
         ```

      b) 新建终端并使用GDB连接qemu

         ```shell
         $ xtensa-esp32-elf-gdb out/esp32/qemu_xtensa_mini_system_demo/OHOS_Image -ex "target remote :1234"
         ```

   注：由于默认安装的qemu自带qemu-system-xtensa工具与当前安装的qemu-system-xtensa工具重名，因此采用绝对路径执行当前的qemu-system-xtensa工具。
   注：qemu退出方式为：按下ctrl加a键，然后松开再按下x键。

(注：更多操作指导，请参考：[Home · espressif/qemu Wiki · GitHub](https://github.com/espressif/qemu/wiki#configure))
