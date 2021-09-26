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
  echo "./qemu_run.sh [Executable file] test [log.txt]"
}

qemu_test=""
if [ $# == 1 ]; then
  elf_file=$1
elif [ $# == 2 ]; then
  if [ "$1" == "gdb" ]; then
    elf_file=$2
    gdb_option="-s -S"
  else
    help_func
    exit
  fi
elif [ $# == 3 ]; then
  if [ "$2" == "test" ]; then
    qemu_test=$2
    elf_file=$1
    out_file=$3
    build_file=$(dirname $out_file)/build.log
    out_option="-serial file:$out_file"
    ./device/qemu/arm_mps2_an386/scripts/qemu_test_monitor.sh $out_file &
  else
    help_func
    exit
  fi
fi

if [ "$elf_file" == "" ]; then
  echo "Specify the path to the executable file"
  help_func
  exit
fi

qemu-system-arm                                   \
  -M mps2-an386                                   \
  -m 16M                                          \
  -kernel $elf_file                               \
  $gdb_option                                     \
  -append "root=dev/vda or console=ttyS0"         \
  $out_option                                     \
  -nographic


function test_success() {
  echo "Test success!!!"
  exit 0
}

function test_failed() {
  cat $out_file >> $build_file
  echo "Test failed!!!"
  exit 1
}

if [ "$qemu_test" = "test" ]; then
  if [ ! -f "$out_file" ]; then
    test_failed
  else
    result=`tail -1 $out_file`
    if [ "$result" != "--- Test End ---" ]; then
      test_failed
    fi

    result=`tail -2 $out_file`
    failedresult=${result%,*}
    failed=${failedresult%:*}
    if [ "$failed" != "failed count" ]; then
      test_failed
    fi

    failedcount=${failedresult#*:}
    if [ "$failedcount" = "0" ]; then
      test_success
    else
      test_failed
    fi
  fi
fi
