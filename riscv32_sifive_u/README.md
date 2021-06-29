### Qemu RISC-V sifive_u HOWTO

#### 1. Brief introduction
`risc-v/` subdirectory contains part of the OpenHarmony LiteOS demonstration support for Qemu risc-v sifive_u Platform,
here called *sifive_u*.
RISC-V Virtual platform is a `qemu-system-riscv32` machine target that provides emulation
for a generic, riscv-based board.

Introduced functionality adds support for RISC-V (1 CPU with security extensions), 128MB memory virtual platform.

Note: System memory size is hard-coded to 128MB.

#### 2. Setting up environment

Refer to HOWTO guide: [Setting up a development environment](https://gitee.com/openharmony/docs/blob/master/en/device-dev/quick-start/environment-setup.md)

#### 3. Code acquisition

Refer to HOWTO guide: [Code acquisition](https://gitee.com/openharmony/docs/blob/master/en/device-dev/get-code/source-code-acquisition.md)

Note: One can use `repo` to fetch code in a straightforward manner.

#### 4. Building from sources

```
cd device/qemu/riscv32_sifive_u
make clean;make -j16
```

This will build `OHOS_Image` for Qemu ARM virt machine.


After build is finished, the resulting image can be found in:
```
out/OHOS_Image
```
#### 5. Running image in Qemu

a) If not installed, please install `qemu-system-riscv32`
For details, please refer to the HOWTO: [Qemu installation](https://www.qemu.org/download/)

Note: The functionality currently introduced has been tested on the target machine based on Qemu version 4.0.90, 
      and there is no guarantee that all Qemu versions will work successfully, so you need to make sure that your
      qemu-system-riscv32 version is 4.0.90 as far as possible.

b) Run

```
cd device/qemu/riscv32_sifive_u
./qemu-system-riscv32 out/OHOS_Image
```