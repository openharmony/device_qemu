# QEMU(Tutorials of C-SKY )

## 1.Introduction

QEMU can simulate the kernel to run on different boards, freeing the dependence on the physical development board.The `SmartL_E802/` subdirectory contains part of the OpenHarmony kernel\_liteos\_m code verified by the Qemu C-SKY virtualization platform,you can simulate a single board based on the C-SKY architecture.

## 2.Setup Environment

   1. Set up the Compilation tool chain

      a) Create the `csky_toolchain` folder and enter it

         ```shell
         $ mkdir csky_toolchain && cd csky_toolchain
         ```

      b) Download  csky-elfabiv2-tools-x86_64-minilibc-20210423.tar.gz to `csky_toolchain`folder and unzip,download link: https://occ.t-head.cn/community/download?id=3885366095506644992

         ```shell
         $ wget https://occ-oss-prod.oss-cn-hangzhou.aliyuncs.com/resource/1356021/1619529111421/csky-elfabiv2-tools-x86_64-minilibc-20210423.tar.gz
         $ tar -xf csky-elfabiv2-tools-x86_64-minilibc-20210423.tar.gz
         ```

      c) Add the csky-elfabiv2 compilation tool chain to the environment variable (modify user_toolchain_xxx_path to your own installation path):

         ```shell
         $ vim ~/.bashrc
         $ export PATH=$PATH:user_toolchain_xxx_path/csky_toolchain/bin
         $ source ~/.bashrc
         ```

   2. Install qemu

      a) creat`csky_qemu`folder and enter it 

         ```shell
         $ mkdir csky_qemu && cd csky_qemu
         ```

      b) download csky-qemu-x86_64-Ubuntu-16.04-20210202-1445.tar.gz to `csky_qemu`folder and unzip,download link: https://occ.t-head.cn/community/download?id=636946310057951232

         ```shell
         $ wget https://occ-oss-prod.oss-cn-hangzhou.aliyuncs.com/resource/1356021/1612269502091/csky-qemu-x86_64-Ubuntu-16.04-20210202-1445.tar.gz
         $ tar -xf csky-qemu-x86_64-Ubuntu-16.04-20210202-1445.tar.gz
         ```

      c) Add qemu to the environment variable (modify user_qemu_xxx_path to your own installation path):

         ```shell
         $ vim ~/.bashrc
         export PATH=$PATH:user_qemu_xxx_path/csky-qemu/bin
         $ source ~/.bashrc
         ```

      d) Installation dependencies

         ```shell
         $ ldd qemu_installation_path/bin/qemu-system-cskyv2
         ```

         According to the execution result of ldd, install the missing dependent libraries

         (Annotation: For more installation instructions, please refer to the following link: https://occ.t-head.cn/community/download?id=636946310057951232)

## 3.Get source code

[code acquisition ](https://gitee.com/openharmony/docs/blob/master/en/device-dev/get-code/sourcecode-acquire.md)

Hint : You can use the `repo` command to get the source code.

## 4.Source buildding

   1. Execute the hb set command and select the project`qemu_csky_mini_system_demo`.

   2. Execute the hb clean && hb build command to build the executable file that produces `liteos`.

      ```shell
      $ hb set
      $ hb clean && hb build
      ```

   3. After the buildding is complete,the corresponding executable file is in the home directory:

      ```
      out/SmartL_E802/qemu_csky_mini_system_demo/bin/
      ```

   Annotation: The current liteos is an elf file without a symbol table,and the path of the elf file with a symbol table is as follows:

      ```
      out/SmartL_E802/qemu_csky_mini_system_demo/unstripped/bin/
      ```

## 5.Run the image in Qemu

   1. Run qemu(Don't cooperate with GDB)

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
         $ csky-abiv2-elf-gdb out/SmartL_E802/qemu_csky_mini_system_demo/unstripped/bin/liteos -ex "target remote localhost:1234"
         ```

   Annotation: Elf files with symbol tables are used by default.
   Annotation: The way to exit qemu : press ctrl and a,then release and press x.
