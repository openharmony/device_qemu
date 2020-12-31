# device_qemu

#### Introduction

The qemu repository is to use QEMU to emulate different hardware platforms.

#### Directory Structure

```
.
└── <generic_hardware_platform>                            --- Generic hardware platforms, eg. cortex-a7，cortex-m3.
    ├── driver                                             --- Driver code
    ├── main.c                                             --- Main function entry
    └── project                                            --- Project settings

```

#### HOWTOs

[QEMU ARM Virt](https://gitee.com/openharmony/device_qemu/blob/master/arm/virt/README.en.md)
