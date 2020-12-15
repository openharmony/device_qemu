
LITEOSTOPDIR := $(shell pwd)/../../../../../kernel/liteos_a
CONFIG_FILE := $(LITEOSTOPDIR)/.config

ifeq ($(wildcard $(CONFIG_FILE)), )
$(error  "########################kernel is not compiled################################")
endif
include $(LITEOSTOPDIR)/config.mk

MPP_DIR := $(shell pwd)/../../virt/module_init/

include virt.mak
