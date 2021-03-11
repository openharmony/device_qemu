#!/bin/bash
# Copyright (c) 2020 Huawei Device Co., Ltd. All rights reserved.
#
# Compile mpp/sample project, this is the entrance script

# error out on errors
set -e
OUT_DIR="$2"
KERNEL_DIR=$1

function main(){
    pushd $KERNEL_DIR
    cp tools/build/config/qemu_arm_virt_debug_shell.config .config
    make clean OUTDIR=$OUT_DIR && make rootfs -e -j 16 OUTDIR=$OUT_DIR
    echo $PWD
    cp -rf $OUT_DIR/rootfs*.* $OUT_DIR/../../../
    cp -rf $OUT_DIR/liteos $OUT_DIR/../../../OHOS_Image
    cp -rf $OUT_DIR/liteos.asm $OUT_DIR/../../../OHOS_Image.asm
    cp -rf $OUT_DIR/liteos.map $OUT_DIR/../../../OHOS_Image.map
    mv -f $OUT_DIR/../../../liteos.bin $OUT_DIR/../../../OHOS_Image.bin
    popd
}
main "$@"
