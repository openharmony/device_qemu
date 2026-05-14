#!/bin/bash

# Copyright (c) 2024 Institute of Software, Chinese Academy of Sciences .

set -e

pushd ${1}

export OHOS_SOURCE_ROOT=${2}
export OHOS_OUT_DIR=${OHOS_SOURCE_ROOT}/out
export OHOS_KERNEL_OBJ=${OHOS_OUT_DIR}
export OHOS_IMAGES_DIR=${3}

export KERNEL_SOURCE_DIR=${4}
export KERNEL_PATCH_PATH=${5}
export KERNEL_CONFIG_NAME=${6}_virt_defconfig

export OUT_DIR=${OHOS_SOURCE_ROOT}/out

export KERNEL_VERSION=${6}_virt
export PATH=${OHOS_SOURCE_ROOT}/prebuilts/clang/ohos/linux-x86_64/llvm/bin:$PATH
export KERNEL_BUILD_ROOT=${OHOS_KERNEL_OBJ}/kernel/OBJ/${KERNEL_VERSION}
export PRODUCT_PATH=vendor/ohemu/virt

if [ ${6} == "arm" ]
then
    export PATH=${OHOS_SOURCE_ROOT}/prebuilts/gcc/linux-x86/arm/gcc-linaro-7.5.0-arm-linux-gnueabi/bin:$PATH
    export MAKE_OPTIONS="ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- "
    export KERNEL_OUT_IMAGE=${KERNEL_BUILD_ROOT}/arch/arm/boot/zImage
elif [ ${6} == "riscv64" ]
then
    export PATH=${OHOS_SOURCE_ROOT}/prebuilts/gcc/linux-x86/riscv64/riscv64-unknown-linux-gnu/bin:$PATH
    export MAKE_OPTIONS="ARCH=riscv CROSS_COMPILE=riscv64-unknown-linux-gnu- "
    export KERNEL_OUT_IMAGE=${KERNEL_BUILD_ROOT}/arch/riscv/boot/Image
elif [ ${6} == "x86_64" ]
then
    export PATH=${OHOS_SOURCE_ROOT}/prebuilts/gcc/linux-x86/riscv64/riscv64-unknown-linux-gnu/bin:$PATH
    export MAKE_OPTIONS=""
    export KERNEL_OUT_IMAGE=${KERNEL_BUILD_ROOT}/arch/x86/boot/bzImage
elif [ ${6} == "arm64" ]
then
    export PATH=${OHOS_SOURCE_ROOT}/prebuilts/gcc/linux-x86/aarch64/gcc-linaro-7.5.0-2019.12-x86_64_aarch64-linux-gnu/bin:$PATH
    export MAKE_OPTIONS="ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- "
    export KERNEL_OUT_IMAGE=${KERNEL_BUILD_ROOT}/arch/arm64/boot/Image
fi

function copy_kernel(){
    rm -rf ${KERNEL_BUILD_ROOT}
    mkdir -p ${OHOS_SOURCE_ROOT}/out/kernel/OBJ
    cp -rL ${KERNEL_SOURCE_DIR}  ${KERNEL_BUILD_ROOT}

    cd ${KERNEL_BUILD_ROOT}

#将drivers/hdf_core/adapter/khdf/linux/patch_hdf.sh拆分为链接和复制
##hdf_ln
    mkdir -p ${KERNEL_BUILD_ROOT}/drivers/hdf
    ln -sf ${OHOS_SOURCE_ROOT}/drivers/hdf_core/adapter/khdf/linux	${KERNEL_BUILD_ROOT}/drivers/hdf/khdf
    ln -sf ${OHOS_SOURCE_ROOT}/drivers/hdf_core/framework		${KERNEL_BUILD_ROOT}/drivers/hdf/framework
    ln -sf ${OHOS_SOURCE_ROOT}/drivers/hdf_core/interfaces/inner_api	${KERNEL_BUILD_ROOT}/drivers/hdf/inner_api
    ln -sf ${OHOS_SOURCE_ROOT}/drivers/hdf_core/framework/include	${KERNEL_BUILD_ROOT}/include/hdf
##hdf_cp
    cp -arfL $OHOS_SOURCE_ROOT/third_party/bounds_checking_function	${KERNEL_BUILD_ROOT}/
    cp -arfL $OHOS_SOURCE_ROOT/third_party/FreeBSD/sys/dev/evdev		${KERNEL_BUILD_ROOT}/drivers/hdf/
##fatal error: stdarg.h
    cp ${KERNEL_BUILD_ROOT}/include/linux/stdarg.h ${KERNEL_BUILD_ROOT}/bounds_checking_function/include

#cp config
    cp ${KERNEL_PATCH_PATH}/${KERNEL_CONFIG_NAME} ${KERNEL_BUILD_ROOT}/.config

#cp common_modules
    cp -arfL  $OHOS_SOURCE_ROOT/kernel/linux/common_modules/memory_security ${KERNEL_BUILD_ROOT}/fs/proc
    cp -arfL  $OHOS_SOURCE_ROOT/kernel/linux/common_modules/code_sign ${KERNEL_BUILD_ROOT}/fs
    cp -arfL  $OHOS_SOURCE_ROOT/kernel/linux/common_modules/dec ${KERNEL_BUILD_ROOT}/fs
#patch
    patch -d ${KERNEL_BUILD_ROOT} -p1 <${KERNEL_PATCH_PATH}/patch/hdf.patch
    patch -d ${KERNEL_BUILD_ROOT} -p1 <${KERNEL_PATCH_PATH}/patch/virt.patch
    patch -d ${KERNEL_BUILD_ROOT} -p1 <${KERNEL_PATCH_PATH}/patch/drivers_staging_android.patch
    patch -d ${KERNEL_BUILD_ROOT} -p1 <${KERNEL_PATCH_PATH}/patch/virtio_gpu.patch
    patch -d ${KERNEL_BUILD_ROOT} -p1 <${KERNEL_PATCH_PATH}/patch/es1370.patch
    patch -d ${KERNEL_BUILD_ROOT} -p1 <${KERNEL_PATCH_PATH}/patch/power.patch
    patch -d ${KERNEL_BUILD_ROOT} -p1 <${KERNEL_PATCH_PATH}/patch/pwm_free.patch
}

function cp_ko(){
#8852a
    cp  ${KERNEL_BUILD_ROOT}/drivers/net/wireless/realtek/rtw89/rtw89_core.ko ${KERNEL_PATCH_PATH}/ko
    cp  ${KERNEL_BUILD_ROOT}/drivers/net/wireless/realtek/rtw89/rtw89_8852a.ko ${KERNEL_PATCH_PATH}/ko
    cp  ${KERNEL_BUILD_ROOT}/drivers/net/wireless/realtek/rtw89/rtw89_8852ae.ko ${KERNEL_PATCH_PATH}/ko
    cp  ${KERNEL_BUILD_ROOT}/drivers/net/wireless/realtek/rtw89/rtw89_pci.ko ${KERNEL_PATCH_PATH}/ko
#mt7601u
    cp  ${KERNEL_BUILD_ROOT}/drivers/net/wireless/mediatek/mt7601u/mt7601u.ko ${KERNEL_PATCH_PATH}/ko
#vwifi
    cp  ${KERNEL_BUILD_ROOT}/drivers/net/wireless/virtual/mac80211_hwsim.ko ${KERNEL_PATCH_PATH}/ko
    cp  ${KERNEL_BUILD_ROOT}/drivers/net/wireless/virtual/virt_wifi.ko ${KERNEL_PATCH_PATH}/ko
#iwlwifi,iwlmvm and deps
    cp  ${KERNEL_BUILD_ROOT}/lib/crypto/libarc4.ko ${KERNEL_PATCH_PATH}/ko
    cp  ${KERNEL_BUILD_ROOT}/net/mac80211/mac80211.ko ${KERNEL_PATCH_PATH}/ko
    cp  ${KERNEL_BUILD_ROOT}/net/wireless/cfg80211.ko ${KERNEL_PATCH_PATH}/ko
    cp  ${KERNEL_BUILD_ROOT}/drivers/net/wireless/intel/iwlwifi/iwlwifi.ko ${KERNEL_PATCH_PATH}/ko
    cp  ${KERNEL_BUILD_ROOT}/drivers/net/wireless/intel/iwlwifi/mvm/iwlmvm.ko ${KERNEL_PATCH_PATH}/ko
}

function clean_hdf() {
    if [ -f "${OHOS_SOURCE_ROOT}/vendor/ohemu/virt/hdf_config/khdf/built-in.a" ]; then
        rm ${OHOS_SOURCE_ROOT}/vendor/ohemu/virt/hdf_config/khdf/built-in.a
    fi
    if [ -f "${OHOS_SOURCE_ROOT}/vendor/ohemu/virt/hdf_config/khdf/.built-in.a.cmd" ]; then
        rm ${OHOS_SOURCE_ROOT}/vendor/ohemu/virt/hdf_config/khdf/.built-in.a.cmd
    fi
    if [ -f "${OHOS_SOURCE_ROOT}/vendor/ohemu/virt/hdf_config/khdf/hdf_hcs.hcb" ]; then
        rm ${OHOS_SOURCE_ROOT}/vendor/ohemu/virt/hdf_config/khdf/hdf_hcs.hcb
    fi
    if [ -f "${OHOS_SOURCE_ROOT}/vendor/ohemu/virt/hdf_config/khdf/hdf_hcs_hex.o" ]; then
        rm ${OHOS_SOURCE_ROOT}/vendor/ohemu/virt/hdf_config/khdf/hdf_hcs_hex.o
    fi
    #if [ -f "${OHOS_SOURCE_ROOT}/vendor/ohemu/virt/hdf_config/khdf/.hdf_hcs_hex.o.d" ]; then
    #    rm ${OHOS_SOURCE_ROOT}/vendor/ohemu/virt/hdf_config/khdf/.hdf_hcs_hex.o.d
    #fi
    #if [ -f "${OHOS_SOURCE_ROOT}/vendor/ohemu/virt/hdf_config/khdf/.hdf_hcs_hex.o.cmd" ]; then
    #    rm ${OHOS_SOURCE_ROOT}/vendor/ohemu/virt/hdf_config/khdf/.hdf_hcs_hex.o.cmd
    #fi
    if [ -f "${OHOS_SOURCE_ROOT}/vendor/ohemu/virt/hdf_config/khdf/modules.order" ]; then
        rm ${OHOS_SOURCE_ROOT}/vendor/ohemu/virt/hdf_config/khdf/modules.order
    fi
    if [ -f "${OHOS_SOURCE_ROOT}/vendor/ohemu/virt/hdf_config/khdf/.modules.order.cmd" ]; then
        rm ${OHOS_SOURCE_ROOT}/vendor/ohemu/virt/hdf_config/khdf/.modules.order.cmd
    fi
    if [ -f "${OHOS_SOURCE_ROOT}/vendor/ohemu/virt/hdf_config/khdf/built-in.a" ]; then
        rm ${OHOS_SOURCE_ROOT}/vendor/ohemu/virt/hdf_config/khdf/built-in.a
    fi
}

#make kernel
function make_kernel(){
    cd  ${KERNEL_BUILD_ROOT}
    # Normalize new Kconfig symbols non-interactively before the parallel build.
    make ${MAKE_OPTIONS} olddefconfig
    clean_hdf
    make ${MAKE_OPTIONS} -j$(nproc)
    mkdir -p ${OHOS_IMAGES_DIR}
    cp ${KERNEL_OUT_IMAGE} ${OHOS_IMAGES_DIR}
    cp_ko
    clean_hdf
}

##main
# Always refresh the kernel copy. This board script has no dependency
# tracking like rk3568, so reusing an old OBJ tree can keep stale patched
# sources and hide fixes made in kernel/linux/linux-6.6.
copy_kernel
make_kernel

popd
