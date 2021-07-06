# QEMU<a name="EN-US_TOPIC_0000001101286951"></a>

-   [Introduction](#section11660541593)
-   [Directory Structure](#section161941989596)
-   [Constraints](#section119744591305)
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
├── riscv32_virt            # RISCV32 architecture
│   ├── driver              # Driver code
│   ├── include             # APIs exposed externally
│   ├── libc                # Basic libc library
│   └── liteos_m            # Configuration related to the LiteOS Cortex-m kernel
```

## Constraints<a name="section119744591305"></a>

QEMU applies only to the OpenHarmony kernel.

## Usage<a name="section169045116126"></a>

For details about the ARM architecture, see  [Qemu ARM Virt HOWTO](https://gitee.com/openharmony/device_qemu/blob/master/arm_virt/README.md).

For details about the RISC-V architecture, see  [Qemu RISC-V Virt HOWTO](https://gitee.com/openharmony/device_qemu/blob/master/riscv32_virt/README.md).

## Contribution<a name="section169045116136"></a>

[How to involve](https://gitee.com/openharmony/docs/blob/master/en/contribute/contribution.md)

[Commit message spec](https://gitee.com/openharmony/device_qemu/wikis/Commit%20message%E8%A7%84%E8%8C%83?sort_id=4042860)

## Repositories Involved<a name="section1371113476307"></a>

[Kernel subsystem](https://gitee.com/openharmony/docs/blob/master/en/readme/kernel.md)

**device\_qemu**

[kernel\_liteos\_a](https://gitee.com/openharmony/kernel_liteos_a/blob/master/README.md)

