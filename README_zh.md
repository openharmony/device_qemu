# QEMU（Quick Emulator）<a name="ZH-CN_TOPIC_0000001101286951"></a>

-   [简介](#section11660541593)
-   [目录](#section161941989596)
-   [约束](#section119744591305)
-   [使用说明](#section169045116126)
-   [相关仓](#section1371113476307)

## 简介<a name="section11660541593"></a>

QEMU可以模拟内核运行在不同的单板，解除对物理开发板的依赖。

## 目录<a name="section161941989596"></a>

```
/device/qemu
├── arm_virt                # arm virt单板
│   └── liteos_a            # 与liteos_a内核相关的配置
│       └── config          # 驱动相关配置
├── drivers                 # 与平台相关的驱动目录
│   └── libs                # 驱动库
│       └── virt            # virt平台
├── riscv32                 # riscv32架构相关
│   ├── driver              # 驱动目录
│   ├── include             # 对外接口存放目录
│   └── libc                # 基础libc库
```

## 约束<a name="section119744591305"></a>

只适用于OpenHarmony内核。

## 使用说明<a name="section169045116126"></a>

arm架构参考[QEMU教程 for arm](https://gitee.com/openharmony/device_qemu/blob/master/arm/virt/README.md)，riscv架构教程待后续更新。

## 相关仓<a name="section1371113476307"></a>

**[device\_qemu](https://gitee.com/openharmony/device_qemu/tree/master)**

