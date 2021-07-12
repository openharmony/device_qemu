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

ifeq ($(BUILD_FROM_SOURCE), y)
    HDF_INCLUDE += -I $(LITEOSTOPDIR)/../../device/$(SOC_COMPANY)/$(SOC_PLATFORM)/liteos_a/config/board/include/
    HDF_INCLUDE += -I $(LITEOSTOPDIR)/../../device/$(SOC_COMPANY)/$(SOC_PLATFORM)/liteos_a/config/board/include/soc
endif

###################### SELF-DEVELOPED DRIVER ######################
LITEOS_BASELIB +=  -lcfiflash -lvirtnet -lfw_cfg
LIB_SUBDIRS    += $(DRIVERS_ROOT)/cfiflash
LIB_SUBDIRS    += $(DRIVERS_ROOT)/virtnet
LIB_SUBDIRS    += $(DRIVERS_ROOT)/fw_cfg

###################### HDF DRIVER ######################
ifeq ($(LOSCFG_DRIVERS_HDF_PLATFORM_UART), y)
    LITEOS_BASELIB += -lhdf_uart
    LIB_SUBDIRS    += $(DRIVERS_ROOT)/uart
endif

# mtd drivers
ifeq ($(LOSCFG_DRIVERS_MTD), y)
    LITEOS_BASELIB    += -lmtd_common
    LITEOS_MTD_SPI_NOR_INCLUDE  +=  -I $(DRIVERS_ROOT)/include/mtd/common/include
endif

    ifeq ($(LOSCFG_DRIVERS_MTD_SPI_NOR), y)
    ifeq ($(LOSCFG_DRIVERS_MTD_SPI_NOR_HISFC350), y)
        NOR_DRIVER_DIR := hisfc350
    else ifeq ($(LOSCFG_DRIVERS_MTD_SPI_NOR_HIFMC100), y)
        NOR_DRIVER_DIR := hifmc100
    endif

    LITEOS_BASELIB   += -lspinor_flash
    LITEOS_MTD_SPI_NOR_INCLUDE  +=  -I $(DRIVERS_ROOT)/include/mtd/spi_nor/include

    ifeq ($(LOSCFG_DRIVERS_MTD_NAND), y)
        NAND_DRIVER_DIR := hifmc100
        LITEOS_BASELIB   += -lnand_flash
        LITEOS_MTD_NAND_INCLUDE  +=  -I $(DRIVERS_ROOT)/mtd/nand/include
    endif
endif
ifeq ($(BUILD_FROM_SOURCE), n)
LITEOS_LD_PATH += -L$(DRIVERS_ROOT)/libs/$(SOC_PLATFORM)
endif
