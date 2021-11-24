# QEMU(Tutorials of ESP32)

## 1.Introduction

QEMU can simulate the kernel to run on different boards ,eliminate dependence on physical development boards. The esp32 subdirectory contains part of the OpenHarmony kernel\_liteos\_m code verified by the Qemu esp32 virtualization platform, it can be used to simulate a single-core esp32 single board.

## 2.Setup Environment

   1. Install esp-idf

      Please refer to the installation instructions: (https://docs.espressif.com/projects/esp-idf/en/release-v4.1/get-started/index.html)

   2. Install esptool

      ```shell
      python -m pip install esptool
      ```

   3. Compile qemu-system-xtensa

      a) Install and compile

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

      b) Waitting for the completion of the compilation and executing the installation command (If the compilation fail, please refer to https://github.com/espressif/qemu/issues/21):

         ```shell
         $ ninja -C build
         ```

      c) Add qemu to the environment variable (modify user_qemu_xxx_path to your own installation path)：

         ```shell
         $ vim ~/.bashrc
         $ export QEMU=user_qemu_xxx_path/qemu/build
         $ source ~/.bashrc
         ```

      d) Installation dependencies

         ```shell
         $ ldd $QEMU/qemu-system-xtensa
         ```

         According to the execution result of ldd, install the missing dependent libraries

         (Annotation: For more installation instructions, please refer to the following link: [Home · espressif/qemu Wiki · GitHub](https://github.com/espressif/qemu/wiki#configure))

## 3.Get source code

[code acquisition ](https://gitee.com/openharmony/docs/blob/master/en/device-dev/get-code/sourcecode-acquire.md)

Hint : You can use the `repo` command to get the source code.

## 4.Source buildding

   1. Execute the hb set command and select the project `qemu_xtensa_mini_system_demo`.

   2. Execute the hb clean && hb build command to build the executable file that produces `OHOS_Image`.

      ```shell
      $ hb set
      $ hb clean && hb build
      ```

   3. After the buildding is complete, the corresponding executable file is in the home directory：

      ```
      out/esp32/qemu_xtensa_mini_system_demo/
      ```

## 5.Run the image in Qemu

   1. Run qemu(Don't cooperate with GDB )

      ```shell
      $ ./qemu-run
      ```

   2. Run qemu(Cooperate with GDB)

      a) Start the GDB server and wait for the connection

         ```shell
         $ ./qemu-run -g
         ```

      b) Create a new terminal and use GDB to connect to qemu

         ```shell
         $ xtensa-esp32-elf-gdb out/esp32/qemu_xtensa_mini_system_demo/OHOS_Image -ex "target remote :1234"
         ```

   Annotation：Since the qemu-system-xtensa tool of qemu has the same name as the qemu-system-xtensa tool of esp32, the absolute path is used to execute the qemu-system-xtensa tool of esp32.
   Annotation：The way to exit qemu : press ctrl and a, then release and press x.

(Annotation：For more operating instructions, please refer to：[Home · espressif/qemu Wiki · GitHub](https://github.com/espressif/qemu/wiki#configure))
