#!/bin/bash

#Copyright (c) 2020-2021 Huawei Device Co., Ltd.
#Licensed under the Apache License, Version 2.0 (the "License");
#you may not use this file except in compliance with the License.
#You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#Unless required by applicable law or agreed to in writing, software
#distributed under the License is distributed on an "AS IS" BASIS,
#WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#See the License for the specific language governing permissions and
#limitations under the License.

set -e

OUT_DIR="$2"
SYSROOT_PATH="$3"
ARCH_CFLAGS="$4"
DEVICE_PATH="$5"
WORK_DIR=$OUT_DIR/../../../
KERNEL_DIR=$1
HISI_LIBS_DIR=../../../hisilicon/drivers/libs/ohos/llvm/hi3518ev300
DRIVERS_LIBS_DIR=../../drivers/libs/virt/

export SYSROOT_PATH
export ARCH_CFLAGS
export DEVICE_PATH

function main(){
    mkdir -p $WORK_DIR/bin
    mkdir -p $WORK_DIR/libs
    cp $HISI_LIBS_DIR/libspinor_flash.a $DRIVERS_LIBS_DIR
    pushd $KERNEL_DIR
    cp tools/build/config/qemu_arm_virt_debug_shell.config .config
    make clean OUTDIR=$OUT_DIR && make rootfs -e -j 16 OUTDIR=$OUT_DIR
    cp -rf $OUT_DIR/rootfs*.* $WORK_DIR/
    cp -rf $OUT_DIR/liteos $WORK_DIR/OHOS_Image
    cp -rf $OUT_DIR/liteos.asm $WORK_DIR/OHOS_Image.asm
    cp -rf $OUT_DIR/liteos.map $WORK_DIR/OHOS_Image.map
    mv -f $WORK_DIR/liteos.bin $WORK_DIR/OHOS_Image.bin
    popd
    rm -rf $DRIVERS_LIBS_DIR/libspinor_flash.a
}
main "$@"
