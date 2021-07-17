### Qemu Arm Cortex-m4 mps2-an386 HOWTO

#### 1. Brief introduction
`arm_mps2_an386/` subdirectory contains part of the OpenHarmony LiteOS demonstration support for Qemu Arm Cortex-m4 mps2-an386 Platform,
here called *arm_mps2_an386*.
cortex-m4 Virtual platform is a `qemu-system-arm` machine target that provides emulation
for a generic, arm-based board.

Introduced functionality adds support for Cortex-m4 (1 CPU with security extensions), 16MB memory virtual platform.

Note: System memory size is hard-coded to 16MB.

#### 2. Setting up environment

[Setting up a development environment](https://gitee.com/openharmony/docs/blob/master/en/device-dev/quick-start/environment-setup.md)

Compiler install

```
sudo apt install gcc-arm-none-eabi
```

#### 3. Code acquisition

[Code acquisition](https://gitee.com/openharmony/docs/blob/master/en/device-dev/get-code/source-code-acquisition.md)

Note: One can use `repo` to fetch code in a straightforward manner.

#### 4. Building from sources

```
cd device/qemu/arm_mps2_an386
hb build -f
```

This will build `liteos` for Qemu Cortex-m4 mps2-an386 machine.


After build is finished, the resulting image can be found in:
```
../../../out/arm_mps2_an386/bin/liteos
```
#### 5. Running image in Qemu

a) If not installed, please install `qemu-system-arm`
For details, please refer to the HOWTO: [Qemu installation](https://gitee.com/openharmony/device_qemu/blob/master/README.md)

b) Run

```
cd device/qemu/arm_mps2_an386
./qemu_run.sh ../../../out/arm_mps2_an386/bin/liteos
```
