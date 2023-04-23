### Qemu RISC-V virt HOWTO

#### 1. Brief introduction
`riscv32_virt/` subdirectory contains part of the OpenHarmony LiteOS demonstration support for Qemu risc-v virt Platform,
here called *riscv32_virt*.
RISC-V Virtual platform is a `qemu-system-riscv32` machine target that provides emulation
for a generic, riscv-based board.

Introduced functionality adds support for RISC-V (1 CPU with security extensions), 128MB memory virtual platform.

Note: System memory size is hard-coded to 128MB.

#### 2. Setting up environment

[Setting up a development environment](https://gitee.com/openharmony/docs/blob/master/en/device-dev/quick-start/Readme-EN.md)

[Compiler install:gcc_riscv32](https://gitee.com/openharmony/docs/blob/master/en/device-dev/quick-start/quickstart-pkg-3861-tool.md),
Note: [Downloadable directly](https://repo.huaweicloud.com/harmonyos/compiler/gcc_riscv32/7.3.0/linux/gcc_riscv32-linux-7.3.0.tar.gz)

#### 3. Code acquisition

[Code acquisition](https://gitee.com/openharmony/docs/blob/HEAD/en/device-dev/get-code/sourcecode-acquire.md)

Note: One can use `repo` to fetch code in a straightforward manner.

#### 4. Building from sources

In the root directory of the obtained source code, run the following command:

```
hb set
```

Select `qemu_riscv_mini_system_demo` under **ohemu**.

Run the following build command:

```
hb build
```

This will build `OHOS_Image` for Qemu RISC-V virt machine.

After build is finished, the resulting image can be found in:
```
out/riscv32_virt/qemu_riscv_mini_system_demo/
```

#### 5. Running image in Qemu

a) If not installed, please install `qemu-system-riscv32`
For details, please refer to the HOWTO: [Qemu installation](https://gitee.com/openharmony/device_qemu/blob/HEAD/README.md)

b) Run

(1) qemu version < 5.0.0

```
cd device/qemu/riscv32_virt
qemu-system-riscv32 -machine virt -m 128M -kernel ../../../out/riscv32_virt/qemu_riscv_mini_system_demo/OHOS_Image -nographic -append "root=dev/vda or console=ttyS0"
```

(2). qemu version >= 5.0.0

Run the `./qemu-run --help` command. The following information is displayed:

```
Usage: ./qemu-run [OPTION]...
Run a OHOS image in qemu according to the options.

    Options:

    -e, --exec file_name     kernel exec file name
    -n, --net-enable         enable net
    -l, --local-desktop      no VNC
    -g, --gdb                enable gdb for kernel
    -t, --test               test mode, exclusive with -g
    -h, --help               print help info

    By default, the kernel exec file is: out/riscv32_virt/qemu_riscv_mini_system_demo/OHOS_Image,
    and net will not be enabled, gpu enabled and waiting for VNC connection at port 5920.
```
By default, the network will not be automatically configured if no parameter is specified, and the default kernel exec file will be used.
If you want to use other kernel exec file, please try `./qemu-run -e [file_name]` to change it.

#### 6. gdb debug

```
cd device/qemu/riscv32_virt
vim liteos_m/config.gni
```

In the modified `board_opt_flags` compiler options:

```
board_opt_flags = []
```

to:

```
board_opt_flags = [ "-g" ]
```

Save and exit, recompile under OHOS root directory:

```
hb build -f
```

In a window to enter the command:

```
./qemu-run -g
```

In another window to enter the command:

```
riscv32-unknown-elf-gdb out/riscv32_virt/qemu_riscv_mini_system_demo/OHOS_Image
(gdb) target remote localhost:1234
(gdb) b main
```

More GDB related debugging can refer to [GDB instruction manual](https://sourceware.org/gdb/current/onlinedocs/gdb).

## 7. Example

- [Transferring Files Using FAT Images](example.md#sectionfatfs)


