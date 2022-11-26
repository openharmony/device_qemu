# QEMU(Tutorials of ESP32)

## 1.Introduction

QEMU can simulate the kernel to run on different boards ,eliminate dependence on physical development boards. The esp32 subdirectory contains part of the OpenHarmony kernel\_liteos\_m code verified by the Qemu esp32 virtualization platform, it can be used to simulate a single-core esp32 single board.

## 2.Setup Environment

   1. Install esp-idf

      Please refer to the installation instructions: (https://docs.espressif.com/projects/esp-idf/en/release-v4.1/get-started/index.html)

      Annotation: The installation of esp-idf can be skipped. The bootloader.bin and partition-table.bin generated have been placed in the vendor\ohemu\qemu_xtensa_mini_system_demo\image folder.

      Annotation: If you skip the installation of esp-idf, you can install the toolchain as follows:

      Note: [Downloadable directly](https://repo.huaweicloud.com/openharmony/compiler/gcc_esp/2019r2-8.2.0/linux/esp-2019r2-8.2.0.zip)

      Optional compiler installation process:

      a) Download the esp official release the SDK package: https://www.espressif.com/zh-hans/support/download/sdks-demos?keys=&field_type_tid%5B%5D=13

      b) Put the downloaded SDK package into the Linux system, go to the directory, and run the following commands:

         ```shell
         unzip esp-idf-v4.3.1.zip
         cd esp-idf-v4.3.1/
         ./install.sh
         . ./export.sh
         ```

      Annotation: The version of the toolchain used in the test is GCC Version 8.2.0 (Crosstool-ng ESP-2019R2) or GCC Version 8.4.0 (Crosstool-ng ESP-2021R1).  

   2. Install esptool

      a) In step 1, the export.sh script will set the esptool in the environment path, you need to ensure that the esptool version is 3.1 and above.
         ```shell
         esptool.py version
         ```
      b) If the version of the esptool that comes with esp-idf is too low, delete the environment variable corresponding to the current esptool path and run the following commands. (The recommended python version is 3.8 and above)
         ```shell
         python -m pip install esptool
         ```

   3. Compile qemu-system-xtensa

      a) Install and compile

         ```shell
         git clone https://github.com/espressif/qemu.git
         cd qemu
         ./configure --target-list=xtensa-softmmu \
            --enable-gcrypt \
            --enable-debug --enable-sanitizers \
            --disable-strip --disable-user \
            --disable-capstone --disable-vnc \
            --disable-sdl --disable-gtk
         ```

      b) Waiting for the completion of the compilation and executing the installation command (If the compilation fail, please refer to https://github.com/espressif/qemu/issues/21):

         ```shell
         ninja -C build
         ```

      c) Add qemu to the environment variable (modify user_qemu_xxx_path to your own installation path)：

         ```shell
         vim ~/.bashrc
         export QEMU=user_qemu_xxx_path/qemu/build
         source ~/.bashrc
         ```

      d) Installation dependencies

         ```shell
         ldd $QEMU/qemu-system-xtensa
         ```

         According to the execution result of ldd, install the missing dependent libraries

         (Annotation: For more installation instructions, please refer to the following link: [Home · espressif/qemu Wiki · GitHub](https://github.com/espressif/qemu/wiki#configure))

## 3.Get source code

[code acquisition ](https://gitee.com/openharmony/docs/blob/master/en/device-dev/get-code/sourcecode-acquire.md)

Hint : You can use the `repo` command to get the source code.

## 4.Source building

   1. Execute the hb set command and select the project `qemu_xtensa_mini_system_demo`.

   2. Execute the hb clean && hb build command to build the executable file that produces `OHOS_Image`.

      ```shell
      hb set
      hb clean && hb build
      ```

   3. After the building is complete, the corresponding executable file is in the home directory：

      ```
      out/esp32/qemu_xtensa_mini_system_demo/
      ```

## 5.Run the image in Qemu

   1. Run qemu(Don't cooperate with GDB )

      ```shell
      ./qemu-run
      ```

   2. Run qemu(Cooperate with GDB)

      a) Start the GDB server and wait for the connection

         ```shell
         ./qemu-run -g
         ```

      b) Create a new terminal and use GDB to connect to qemu

         ```shell
         xtensa-esp32-elf-gdb out/esp32/qemu_xtensa_mini_system_demo/OHOS_Image -ex "target remote :1234"
         ```

   Annotation：Since the qemu-system-xtensa tool of qemu has the same name as the qemu-system-xtensa tool of esp32, the absolute path is used to execute the qemu-system-xtensa tool of esp32.
   Annotation：The way to exit qemu : press ctrl and a, then release and press x.

(Annotation：For more operating instructions, please refer to：[Home · espressif/qemu Wiki · GitHub](https://github.com/espressif/qemu/wiki#configure))
