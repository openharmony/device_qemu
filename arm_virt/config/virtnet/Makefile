include $(LITEOSTOPDIR)/../../drivers/adapter/khdf/liteos/lite.mk
MODULE_NAME := $(notdir $(shell pwd))
LOCAL_INCLUDE := -I $(LITEOSTOPDIR)/../../third_party/lwip/src/include/
LOCAL_SRCS := virtnet.c virtmmio.c
LOCAL_CFLAGS += $(LOCAL_INCLUDE)

include $(HDF_DRIVER)
