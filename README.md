# QEMU<a name="EN-US_TOPIC_0000001101286951"></a>

-   [Introduction](#section11660541593)
-   [Directory Structure](#section161941989596)
-   [Constraints](#section119744591305)
-   [Usage](#section169045116126)
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
├── riscv32                 # RISCV32 architecture
│   ├── driver              # Driver code
│   ├── include             # APIs exposed externally
│   └── libc                # Basic libc library
```

## Constraints<a name="section119744591305"></a>

QEMU applies only to the OpenHarmony kernel.

## Usage<a name="section169045116126"></a>

For details about the ARM architecture, see  [Qemu ARM Virt HOWTO](https://gitee.com/openharmony/device_qemu/blob/master/arm/virt/README.en.md). The RISCV architecture tutorial will be updated later.

## Repositories Involved<a name="section1371113476307"></a>

**[device\_qemu](https://gitee.com/openharmony/device_qemu/tree/master)**

