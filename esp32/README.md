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
         $ ldd qemu_installation_path/qemu-system-xtensa
         ```

         According to the execution result of ldd, install the missing dependent libraries

         (Annotation: For more installation instructions, please refer to the following link: [Home · espressif/qemu Wiki · GitHub](https://github.com/espressif/qemu/wiki#configure))

## 3.Get the harmony source code

[code acquisition ](https://gitee.com/openharmony/docs/blob/master/en/device-dev/get-code/sourcecode-acquire.md)

Hint : You can use the `repo` command to get the source code.

## 4.Source buildding

   1. Execute the hb set command and select the project `qemu_xtensa_mini_system_demo`.

   2. Execute the hb clean && hb build command to build the executable file that produces `liteos`.

      ```shell
      $ hb set
      $ hb clean && hb build
      ```

   3. After the buildding is complete, the corresponding executable file is in the home directory：

      ```
      out/esp32/qemu_xtensa_mini_system_demo/bin/
      ```

   Annotation：The current liteos is an elf file without a symbol table, and the path of the elf file with a symbol table is as follows:

      ```
      out/esp32/qemu_xtensa_mini_system_demo/unstripped/bin/
      ```

## 5.Run the image in Qemu

   1. Set the QEMU_XTENSA_CORE_REGS_ONLY environment variable

      ```shell
      export QEMU_XTENSA_CORE_REGS_ONLY=1
      ```

   2. Use the elf2image command of esptool.py to convert the liteos file compiled under harmony into a liteos.bin file.

      ```shell
      esptool.py --chip esp32 elf2image --flash_mode dio --flash_freq 80m --flash_size 4MB -o out/esp32/qemu_xtensa_mini_system_demo/bin/liteos.bin out/esp32/qemu_xtensa_mini_system_demo/bin/liteos
      ```

      Annotation：The current liteos is an elf file without a symbol table. If you want to use an elf file with a symbol table, you can use the following command to replace this command.

      ```shell
      esptool.py --chip esp32 elf2image --flash_mode dio --flash_freq 80m --flash_size 4MB -o out/esp32/qemu_xtensa_mini_system_demo/unstripped/bin/liteos.bin out/esp32/qemu_xtensa_mini_system_demo/unstripped/bin/liteos
      ```

   3. Use the merge_bin command of esptool.py. A flash_image.bin will be generated using bootloader.bin compiled by esp-idf, partition-table.bin and liteos.bin compiled under harmony 

      ```shell
      esptool.py --chip esp32 merge_bin --fill-flash-size 4MB -o flash_image.bin 0x1000 bootloader.bin 0x8000 partition-table.bin 0x10000 liteos.bin
      ```

   4. Run qemu(Don't cooperate with GDB )

      ```shell
      $ QEMU/qemu-system-xtensa -nographic -machine esp32 -drive file=flash_image.bin,if=mtd,format=raw
      ```

   Annotation：Since the qemu-system-xtensa tool of qemu has the same name as the qemu-system-xtensa tool of esp32, the absolute path is used to execute the qemu-system-xtensa tool of esp32.

   5. Run qemu(Cooperate with GDB)

      a) Start the GDB server and wait for the connection

         ```
         $ QEMU/qemu-system-xtensa -nographic -s -S -machine esp32 -drive file=flash_image.bin,if=mtd,format=raw
         ```

      b) Create a new terminal and use GDB to connect to qemu
         ```
         $ xtensa-esp32-elf-gdb liteos -ex "target remote :1234"
         ```

   Annotation：If you use GDB for debugging, it is recommended to use the elf file with a symbol table in step 2 of running the mirror.
  Annotation：The way to exit qemu : press ctrl and a, then release and press x.

(Annotation：For more operating instructions, please refer to：[Home · espressif/qemu Wiki · GitHub](https://github.com/espressif/qemu/wiki#configure))
