# QEMU（Quick Emulator）<a name="ZH-CN_TOPIC_0000001101286951"></a>

-   [简介](#section11660541593)
-   [目录](#section161941989596)
-   [约束](#section119744591305)
-   [QEMU安装](#section119744591307)
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
├── riscv32_virt            # riscv32 virt单板
│   ├── driver              # 驱动目录
│   ├── include             # 对外接口存放目录
│   ├── libc                # 基础libc库
│   ├── fs                  # fs 配置
│   ├── test                # 测试样例
│   └── liteos_m            # 与liteos_m内核相关的配置
├── arm_mps2_an386          # cortex-m4 mps2_an386单板
│   ├── driver              # 驱动目录
│   ├── include             # 对外接口存放目录
│   ├── libc                # 基础libc库
│   ├── fs                  # fs 配置
│   ├── test                # 测试样例
│   └── liteos_m            # 与liteos_m内核相关的配置
├── esp32                   # Xtensa LX6 esp32单板
│   ├── hals                # 硬件适配层
│   ├── driver              # 驱动目录
│   ├── include             # 对外接口存放目录
│   ├── libc                # 基础libc库
│   ├── fs                  # fs 配置
│   ├── test                # 测试样例
│   └── liteos_m            # 与liteos_m内核相关的配置
├── SmartL_E802             # C-SKY SmartL虚拟单板
│   ├── hals                # 硬件适配层
│   ├── driver              # 驱动目录
│   ├── libc                # 基础libc库
│   ├── fs                  # fs 配置
│   ├── test                # 测试样例
│   └── liteos_m            # 与liteos_m内核相关的配置
```

## 约束<a name="section119744591305"></a>

只适用于OpenHarmony内核。

## QEMU安装<a name="section119744591307"></a>

1. 安装依赖(Ubuntu 18+)

   ```
   $ sudo apt install build-essential zlib1g-dev pkg-config libglib2.0-dev  binutils-dev libboost-all-dev autoconf libtool libssl-dev libpixman-1-dev virtualenv flex bison
   ```

2. 获取源码

   ```
   $ wget https://download.qemu.org/qemu-6.0.0.tar.xz
   ```

   或

   [官网下载: qemu-6.0.0.tar.xz](https://download.qemu.org/qemu-6.0.0.tar.xz)

3. 编译安装

   ```
   $ tar -xf qemu-6.0.0.tar.xz
   $ cd qemu-6.0.0
   $ mkdir build && cd build
   $ ../configure --prefix=qemu_installation_path
   $ make -j16
   ```

   等待编译结束, 执行安装命令:

   ```
   $ make install
   ```

   最后将安装路径添加到环境变量中:

   ```
   $ vim ~/.bashrc
   ```

   在~/.bashrc最末尾加入:

   ```
   $ export PATH=$PATH:qemu_installation_path
   ```

## 使用说明<a name="section169045116126"></a>

arm架构参考[QEMU教程 for arm - liteos_a](https://gitee.com/openharmony/device_qemu/blob/HEAD/arm_virt/liteos_a/README_zh.md)。

cortex-m4架构参考[QEMU教程 for cortex-m4](https://gitee.com/openharmony/device_qemu/blob/HEAD/arm_mps2_an386/README_zh.md)。

risc-v架构参考[QEMU教程 for risc-v](https://gitee.com/openharmony/device_qemu/blob/HEAD/riscv32_virt/README_zh.md)。

Xtensa架构参考[QEMU教程 for Xtensa](https://gitee.com/openharmony/device_qemu/blob/HEAD/esp32/README_zh.md)。

C-SKY架构参考[QEMU教程 for C-SKY](https://gitee.com/openharmony/device_qemu/blob/HEAD/SmartL_E802/README_zh.md)。

## 贡献<a name="section169045116136"></a>

[如何参与](https://gitee.com/openharmony/docs/blob/HEAD/zh-cn/contribute/%E5%8F%82%E4%B8%8E%E8%B4%A1%E7%8C%AE.md)

[Commit message规范](https://gitee.com/openharmony/device_qemu/wikis/Commit%20message%E8%A7%84%E8%8C%83?sort_id=4042860)

## 相关仓<a name="section1371113476307"></a>

[内核子系统](https://gitee.com/openharmony/docs/blob/HEAD/zh-cn/readme/%E5%86%85%E6%A0%B8%E5%AD%90%E7%B3%BB%E7%BB%9F.md)

**device\_qemu**

[kernel\_liteos\_a](https://gitee.com/openharmony/kernel_liteos_a/blob/HEAD/README_zh.md)

[kernel\_liteos\_m](https://gitee.com/openharmony/kernel_liteos_m/blob/HEAD/README_zh.md)
