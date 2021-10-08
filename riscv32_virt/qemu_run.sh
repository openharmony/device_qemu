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

#set -e
GDBOPTIONS=""
net_enable=no
EXEFILE=../../../out/riscv32_virt/qemu_riscv_mini_system_demo/bin/liteos

help_info=$(cat <<-END
Usage: $0 [OPTION]...
Run a OHOS image in qemu according to the options.

    Options:

    -f, --file [file_name]   kernel exec file name
    -n, --net-enable         enable net
    -g, --gdb                enable gdb for kernel
    -h, --help               print help info

    By default, the kernel exec file is: ${EXEFILE}, 
    and net will not be enabled.
END
)

function net_config(){
    echo "Network config..."
    sudo modprobe tun tap
    sudo ip link add br0 type bridge
    sudo ip address add 10.0.2.2/24 dev br0
    sudo ip link set dev br0 up
}

function start_qemu(){
    net_enable=${1}
    read -t 5 -p "Enter to start qemu[y/n]:" flag
    start=${flag:-y}
    if [ ${start} = y ]; then
        if [ ${net_enable} = yes ]
        then
            net_config 2>/dev/null
            sudo `which qemu-system-riscv32` -M virt -m 128M -bios none -nographic -kernel $EXEFILE \
            -global virtio-mmio.force-legacy=false \
            $GDBOPTIONS \
            -netdev bridge,id=net0 \
            -device virtio-net-device,netdev=net0,mac=12:22:33:44:55:66 \
            -append "root=/dev/vda or console=ttyS0"
        else
            `which qemu-system-riscv32` -M virt -m 128M -bios none -nographic -kernel $EXEFILE \
            $GDBOPTIONS \
            -append "root=/dev/vda or console=ttyS0"
        fi
    else
        echo "Exit qemu-run"
    fi
}

############################## main ##############################
ARGS=`getopt -o f:ngh --l file:,net-enable,gdb,help -n "$0" -- "$@"`
if [ $? != 0 ]; then
    echo "Try '$0 --help' for more information."
    exit 1
fi
eval set --"${ARGS}"

while true;do
    case "${1}" in
        -f|--file)
        EXEFILE="${2}"
        shift;
        shift;
        ;;
        -n|--net-enable)
        shift;
        net_enable=yes
        echo -e "Qemu net enable..."
        ;;
        -g|--gdb)
        shift;
        GDBOPTIONS="-s -S"
        echo -e "Qemu kernel gdb enable..."
        ;;
        -h|--help)
        shift;
        echo -e "${help_info}"
        exit
        ;;
        --)
        shift;
        break;
        ;;
    esac
done

if [ "$EXEFILE" == "" ] || [ ! -f "${EXEFILE}" ]; then
  echo "Specify the path to the executable file"
  echo -e "${help_info}"
  exit
fi
echo -e "EXEFILE = ${EXEFILE}"
start_qemu ${net_enable}
