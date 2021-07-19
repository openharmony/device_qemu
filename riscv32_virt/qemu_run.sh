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

function help_func() {
  echo "For example:"
  echo "./qemu_run.sh [Executable file]"
  echo "./qemu_run.sh gdb [Executable file]"
}

if [ $# == 1 ]; then
  EXEFILE=$1
elif [ $# == 2 ]; then
  EXEFILE=$2
  if [ "$1" == "gdb" ]; then
    GDBOPTIONS="-s -S"
  else
    help_func
    exit
  fi
fi

if [ "$EXEFILE" == "" ]; then
  echo "Specify the path to the executable file"
  help_func
  exit
fi

qemu-system-riscv32        \
  -m 128M                  \
  -bios none               \
  -machine virt            \
  -kernel $EXEFILE         \
  -nographic               \
  $GDBOPTIONS              \
  -append "root=/dev/vda or console=ttyS0"
