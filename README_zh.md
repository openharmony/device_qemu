# QEMU（Quick Emulator）<a name="ZH-CN_TOPIC_0000001101286951"></a>

-   [简介](#section11660541593)
-   [目录](#section161941989596)
-   [约束](#section119744591305)
-   [使用说明](#section169045116126)
-   [贡献](#section169045116136)
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
├── riscv32_virt            # riscv32架构相关
│   ├── driver              # 驱动目录
│   ├── include             # 对外接口存放目录
│   ├── libc                # 基础libc库
│   └── liteos_m            # 与liteos_m内核相关的配置
```

## 约束<a name="section119744591305"></a>

只适用于OpenHarmony内核。

## 使用说明<a name="section169045116126"></a>

arm架构参考[QEMU教程 for arm](https://gitee.com/openharmony/device_qemu/blob/master/arm_virt/README_zh.md)。

risc-v架构参考[QEMU教程 for risc-v](https://gitee.com/openharmony/device_qemu/blob/master/riscv32_virt/README_zh.md)。

## 贡献<a name="section169045116136"></a>

[如何参与](https://gitee.com/openharmony/docs/blob/master/zh-cn/contribute/%E5%8F%82%E4%B8%8E%E8%B4%A1%E7%8C%AE.md)

[Commit message规范](https://gitee.com/openharmony/device_qemu/wikis/Commit%20message%E8%A7%84%E8%8C%83?sort_id=4042860)

## 相关仓<a name="section1371113476307"></a>

[内核子系统](https://gitee.com/openharmony/docs/blob/master/zh-cn/readme/%E5%86%85%E6%A0%B8%E5%AD%90%E7%B3%BB%E7%BB%9F.md)

**device\_qemu**

[kernel\_liteos\_a](https://gitee.com/openharmony/kernel_liteos_a/blob/master/README_zh.md)

