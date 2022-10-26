#Copyright (c) 2020-2021 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

SOC_COMPANY := $(subst $\",,$(LOSCFG_DEVICE_COMPANY))
SOC_PLATFORM := $(subst $\",,$(LOSCFG_PLATFORM))

DRIVERS_ROOT := $(LITEOSTOPDIR)/../../device/$(SOC_COMPANY)/drivers/

###################### SELF-DEVELOPED DRIVER ######################
LITEOS_BASELIB +=  -lvirtio -lplatform_char
LIB_SUBDIRS    += $(DRIVERS_ROOT)/virtio
LIB_SUBDIRS    += $(DRIVERS_ROOT)/char

###################### HDF DRIVER ######################
ifeq ($(LOSCFG_DRIVERS_HDF_PLATFORM_UART), y)
    LITEOS_BASELIB += -lhdf_uart
    LIB_SUBDIRS    += $(DRIVERS_ROOT)/uart
endif
