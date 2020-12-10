# device_qemu

#### 介绍

使用qemu模拟不同内核运行在不同的单板，解除对物理开发板的依赖。

#### 目录结构

```
.
└── <generic_hardware_platform>                            --- <通用硬件平台>，采用QEMU模拟该通用硬件平台，例如cortex-a7，cortex-m3等
    ├── driver                                             --- 驱动目录（可选）
    ├── main.c                                             --- main函数入口
    └── project                                            --- 编译构建工程目录

```
