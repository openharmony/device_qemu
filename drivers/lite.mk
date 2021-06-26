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

HISILICON_DRIVERS_ROOT := $(LITEOSTOPDIR)/../../device/$(SOC_COMPANY)/drivers/
HISILICON_DRIVERS_SOURCE_ROOT := $(LITEOSTOPDIR)/../../device/$(SOC_COMPANY)/drivers/huawei_proprietary/

BUILD_FROM_SOURCE := $(shell if [ -d $(HISILICON_DRIVERS_SOURCE_ROOT) ]; then echo y; else echo n; fi)

ifeq ($(BUILD_FROM_SOURCE), y)
    HDF_INCLUDE += -I $(LITEOSTOPDIR)/../../device/$(SOC_COMPANY)/$(SOC_PLATFORM)/liteos_a/config/board/include/
    HDF_INCLUDE += -I $(LITEOSTOPDIR)/../../device/$(SOC_COMPANY)/$(SOC_PLATFORM)/liteos_a/config/board/include/soc
endif

###################### SELF-DEVELOPED DRIVER ######################
LITEOS_BASELIB +=  -lcfiflash -lvirtnet -lfw_cfg
LIB_SUBDIRS    += $(HISILICON_DRIVERS_ROOT)/cfiflash
LIB_SUBDIRS    += $(HISILICON_DRIVERS_ROOT)/virtnet
LIB_SUBDIRS    += $(HISILICON_DRIVERS_ROOT)/fw_cfg

###################### HDF DRIVER ######################
ifeq ($(LOSCFG_DRIVERS_HDF_PLATFORM_UART), y)
    LITEOS_BASELIB += -lhdf_uart
    LIB_SUBDIRS    += $(HISILICON_DRIVERS_ROOT)/uart
endif

# mtd drivers
ifeq ($(LOSCFG_DRIVERS_MTD), y)
    LITEOS_BASELIB    += -lmtd_common
ifeq ($(BUILD_FROM_SOURCE), y)
    LIB_SUBDIRS       += $(HISILICON_DRIVERS_SOURCE_ROOT)/mtd/common
    LITEOS_MTD_SPI_NOR_INCLUDE  +=  -I $(HISILICON_DRIVERS_SOURCE_ROOT)/mtd/common/include
else
    LITEOS_MTD_SPI_NOR_INCLUDE  +=  -I $(HISILICON_DRIVERS_ROOT)/include/mtd/common/include
endif

    ifeq ($(LOSCFG_DRIVERS_MTD_SPI_NOR), y)
    ifeq ($(LOSCFG_DRIVERS_MTD_SPI_NOR_HISFC350), y)
        NOR_DRIVER_DIR := hisfc350
    else ifeq ($(LOSCFG_DRIVERS_MTD_SPI_NOR_HIFMC100), y)
        NOR_DRIVER_DIR := hifmc100
    endif

    ifeq ($(BUILD_FROM_SOURCE), y)
        LITEOS_BASELIB   += -lspinor_flash
        LIB_SUBDIRS      += $(HISILICON_DRIVERS_SOURCE_ROOT)/mtd/spi_nor
        LITEOS_MTD_SPI_NOR_INCLUDE  +=  -I $(HISILICON_DRIVERS_SOURCE_ROOT)/mtd/spi_nor/include
    else
        LITEOS_BASELIB   += -lspinor_flash
        LITEOS_MTD_SPI_NOR_INCLUDE  +=  -I $(HISILICON_DRIVERS_ROOT)/include/mtd/spi_nor/include
    endif

    endif

    ifeq ($(LOSCFG_DRIVERS_MTD_NAND), y)
        NAND_DRIVER_DIR := hifmc100

        LITEOS_BASELIB   += -lnand_flash
        LIB_SUBDIRS      += $(HISILICON_DRIVERS_SOURCE_ROOT)/mtd/nand
        LITEOS_MTD_NAND_INCLUDE  +=  -I $(HISILICON_DRIVERS_ROOT)/mtd/nand/include
    endif
endif
ifeq ($(BUILD_FROM_SOURCE), n)
LITEOS_LD_PATH += -L$(HISILICON_DRIVERS_ROOT)/libs/$(SOC_PLATFORM)
endif
