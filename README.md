# QEMU<a name="EN-US_TOPIC_0000001101286951"></a>

-   [Introduction](#section11660541593)
-   [Directory Structure](#section161941989596)
-   [Constraints](#section119744591305)
-   [QEMU Install](#section119744591307)
-   [Usage](#section169045116126)
-   [Contribution](#section169045116136)
-   [Repositories Involved](#section1371113476307)

## Introduction<a name="section11660541593"></a>

Quick Emulator \(QEMU\) can simulate the scenario where a kernel runs on different boards, so that the kernel no longer depends on physical development boards.

## Directory Structure<a name="section161941989596"></a>

```
/device/qemu
├── arm_virt                # ARM virt board
│   └── liteos_a            # Configuration related to the LiteOS Cortex-A kernel
│       └── config          # Configuration related to drivers
├── drivers                 # Platform drivers
│   └── libs                # Driver library
│       └── virt            # virt platform
├── riscv32_virt            # RISCV32 virt board
│   ├── driver              # Driver code
│   ├── include             # APIs exposed externally
│   ├── libc                # Basic libc library
│   ├── fs                  # fs configuration
│   ├── test                # Test the demo
│   └── liteos_m            # Configuration related to the LiteOS Cortex-m kernel
├── arm_mps2_an386          # cortex-m4 mps2_an386 board
│   ├── driver              # Driver code
│   ├── include             # APIs exposed externally
│   ├── libc                # Basic libc library
│   ├── fs                  # fs configuration
│   ├── test                # Test the demo
│   └── liteos_m            # Configuration related to the LiteOS Cortex-m kernel
├── arm_mps3_an547          # cortex-m55 mps3_an547 board
│   ├── driver              # Driver code
│   ├── include             # APIs exposed externally
│   ├── libc                # Basic libc library
│   ├── fs                  # fs configuration
│   ├── test                # Test the demo
│   └── liteos_m            # Configuration related to the LiteOS Cortex-m kernel
├── esp32                   # Xtensa esp32 board
│   ├── hals                # Hardware adaptation layer
│   ├── driver              # Driver code
│   ├── include             # APIs exposed externally
│   ├── libc                # Basic libc library
│   ├── fs                  # fs configuration
│   ├── test                # Test the demo
│   └── liteos_m            # Configuration related to the LiteOS Cortex-m kernel
├── SmartL_E802             # C-SKY SmartL_E802 board
│   ├── hals                # Hardware adaptation layer
│   ├── driver              # Driver code
│   ├── libc                # Basic libc library
│   ├── fs                  # fs configuration
│   ├── test                # Test the demo
│   └── liteos_m            # Configuration related to the LiteOS Cortex-m kernel
```

## Constraints<a name="section119744591305"></a>

QEMU applies only to the OpenHarmony kernel.

## QEMU Install<a name="section119744591307"></a>

1. Install dependencies(Ubuntu 18+)

   ```
   sudo apt install build-essential zlib1g-dev pkg-config libglib2.0-dev  binutils-dev libboost-all-dev autoconf libtool libssl-dev libpixman-1-dev virtualenv flex bison
   ```

2. Acquiring Source Code

   ```
   wget https://download.qemu.org/qemu-6.2.0.tar.xz
   ```

   or

   [Download from official website: qemu-6.2.0.tar.xz](https://download.qemu.org/qemu-6.2.0.tar.xz)

3. Compile and install

   ```
   tar -xf qemu-6.2.0.tar.xz
   cd qemu-6.2.0
   mkdir build && cd build
   ../configure --prefix=qemu_installation_path
   make -j16
   ```

   Wait for the compilation to finish and execute the installation command:

   ```
   make install
   ```

   Finally, add the installation path to the environment variable:

   ```
   vim ~/.bashrc
   ```

   Add the following command line to the last line of ~/.bashrc:

   ```
   export PATH=$PATH:qemu_installation_path
   ```

## Usage<a name="section169045116126"></a>

For details about the ARM architectures:
- [Qemu ARM Virt HOWTO - liteos_a](https://gitee.com/openharmony/device_qemu/blob/HEAD/arm_virt/liteos_a/README.md)
- [Qemu ARM Virt HOWTO - linux](https://gitee.com/openharmony/device_qemu/blob/HEAD/arm_virt/linux/README.md)

For details about the Cortex-m4 architecture, see  [Qemu Cortex-m4 mps2-an386 HOWTO](https://gitee.com/openharmony/device_qemu/blob/HEAD/arm_mps2_an386/README.md).

For details about the Cortex-m55 architecture, see  [Qemu Cortex-m55 mps3-an547 HOWTO](https://gitee.com/openharmony/device_qemu/blob/HEAD/arm_mps3_an547/README.md).

For details about the RISC-V architecture, see  [Qemu RISC-V Virt HOWTO](https://gitee.com/openharmony/device_qemu/blob/HEAD/riscv32_virt/README.md).

For details about the Xtensa architecture, see  [Qemu Xtensa Virt HOWTO](https://gitee.com/openharmony/device_qemu/blob/HEAD/esp32/README.md).

For details about the C-SKY architecture, see  [Qemu C-SKY Virt HOWTO](https://gitee.com/openharmony/device_qemu/blob/HEAD/SmartL_E802/README.md).

## Contribution<a name="section169045116136"></a>

[How to involve](https://gitee.com/openharmony/docs/blob/HEAD/en/contribute/contribution.md)

[Commit message spec](https://gitee.com/openharmony/device_qemu/wikis/Commit%20message%E8%A7%84%E8%8C%83?sort_id=4042860)

## Repositories Involved<a name="section1371113476307"></a>

[Kernel subsystem](https://gitee.com/openharmony/docs/blob/HEAD/en/readme/kernel.md)

**device\_qemu**

[kernel\_liteos\_a](https://gitee.com/openharmony/kernel_liteos_a/blob/HEAD/README.md)

[kernel\_liteos\_m](https://gitee.com/openharmony/kernel_liteos_m/blob/HEAD/README.md)
