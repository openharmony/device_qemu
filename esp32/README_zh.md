# QEMU（ESP32 教程）

## 1.简介

QEMU可以模拟内核运行在不同的单板，解除对物理开发板的依赖。esp32子目录包含部分Qemu esp32虚拟化平台验证的OpenHarmony kernel\_liteos\_m的代码，它可以用来模拟单核esp32单板。

## 2.环境搭建

   1. esp-idf安装

      使用安装指导请参考：(https://docs.espressif.com/projects/esp-idf/zh_CN/release-v4.1/get-started/index.html)

   2. esptool安装

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
         $ ldd qemu_installation_path/qemu-system-xtensa
         ```

         根据ldd执行结果，安装缺少的依赖库

         (注：更多安装指导，请参考链接：[Home · espressif/qemu Wiki · GitHub](https://github.com/espressif/qemu/wiki#configure))

## 3.获取harmony源码

[代码获取](https://gitee.com/openharmony/docs/blob/master/zh-cn/device-dev/get-code/sourcecode-acquire.md)

提示: 可以使用 `repo` 命令来获取源码。

## 4.源码构建

   1. 执行hb set命令并选择项目`qemu_xtensa_mini_system_demo`。

   2. 执行hb clean && hb build命令构建产生 `liteos` 的可执行文件。

      ```shell
      $ hb set
      $ hb clean && hb build
      ```

   3. 在构建完成之后，对应的可执行文件在主目录下：

      ```
      out/esp32/qemu_xtensa_mini_system_demo/bin/
      ```

   注：当前的liteos为不带符号表的elf文件，带符号表的elf文件路径如下：

      ```
      out/esp32/qemu_xtensa_mini_system_demo/unstripped/bin/
      ```

## 5.在Qemu中运行镜像

   1. 设置QEMU_XTENSA_CORE_REGS_ONLY环境变量

      ```shell
      export QEMU_XTENSA_CORE_REGS_ONLY=1
      ```

   2. 使用esptool.py的elf2image命令，将harmony下编译生成的liteos文件转换成liteos.bin文件。

      ```shell
      esptool.py --chip esp32 elf2image --flash_mode dio --flash_freq 80m --flash_size 4MB -o out/esp32/qemu_xtensa_mini_system_demo/bin/liteos.bin out/esp32/qemu_xtensa_mini_system_demo/bin/liteos
      ```

      注：当前的liteos为不带符号表的elf文件，如想使用带符号表的elf文件可以使用下列指令替换该指令。

      ```shell
      esptool.py --chip esp32 elf2image --flash_mode dio --flash_freq 80m --flash_size 4MB -o out/esp32/qemu_xtensa_mini_system_demo/unstripped/bin/liteos.bin out/esp32/qemu_xtensa_mini_system_demo/unstripped/bin/liteos
      ```

   3. 使用esptool.py的merge_bin命令，将使用esp-idf编译的bootloader.bin，partition-table.bin以及harmony下编译生成的liteos.bin合成一个flash_image.bin

      ```shell
      esptool.py --chip esp32 merge_bin --fill-flash-size 4MB -o flash_image.bin 0x1000 bootloader.bin 0x8000 partition-table.bin 0x10000 liteos.bin
      ```

   4. 启动qemu(不配合GDB)

      ```shell
      $ $QEMU/qemu-system-xtensa -nographic -machine esp32 -drive file=flash_image.bin,if=mtd,format=raw
      ```

   注：由于默认安装的qemu自带qemu-system-xtensa工具与当前安装的qemu-system-xtensa工具重名，因此采用绝对路径执行当前的qemu-system-xtensa工具。

   5. 启动qemu(配合GDB)

      a) 启动GDB服务器，等待连接

         ```
         $ $QEMU/qemu-system-xtensa -nographic -s -S -machine esp32 -drive file=flash_image.bin,if=mtd,format=raw
         ```

      b) 新建终端并使用GDB连接qemu
         ```
         $ xtensa-esp32-elf-gdb liteos -ex "target remote :1234"
         ```

   注：如果使用GDB调试，建议在运行镜像的步骤2中使用带符号表的elf文件。
   注：qemu退出方式为：按下ctrl加a键，然后松开再按下x键。

(注：更多操作指导，请参考：[Home · espressif/qemu Wiki · GitHub](https://github.com/espressif/qemu/wiki#configure))
