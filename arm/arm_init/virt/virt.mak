# Qemu ARM Virtual Platform
REL_LIB := $(MPP_DIR)/lib
REL_INC := $(MPP_DIR)/include
ARCH_LIBNAME := virt

#########################################################################

TARGET := OHOS_Image

TARGET_PATH := $(PWD)

# compile OHOS
# TODO

LITEOS_LIBDEPS = --start-group $(LITEOS_LIBDEP) --end-group $(LITEOS_TABLES_LDFLAGS)

LDFLAGS := -L$(OUTDIR)/obj/kernel/liteos_a/lib $(LITEOS_LDFLAGS) --gc-sections

# target source
SRCS  := $(MPP_DIR)/src/system_init.c

OBJS  := $(SRCS:%.c=%.o)
OBJS += $(COMM_OBJ)

BIN := $(TARGET_PATH)/$(TARGET).bin
MAP := $(TARGET_PATH)/$(TARGET).map

all: $(BIN)

$(BIN):$(TARGET)
	@$(OBJCOPY) -O binary $(TARGET_PATH)/$(TARGET) $(BIN)
	cp $(TARGET_PATH)/$(TARGET)* $(OUTDIR)

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) -Map=$(MAP) -o $(TARGET_PATH)/$(TARGET) $(OBJS) $(LITEOS_LIBDEPS)
	@$(OBJDUMP) -d $(TARGET_PATH)/$(TARGET) > $(TARGET_PATH)/$(TARGET).asm

$(OBJS):%.o:%.c
	@$(CC) $(CFLAGS) $(LITEOS_CFLAGS) -c $< -o $@

clean:
	@rm -f $(TARGET_PATH)/$(TARGET) $(BIN) $(MAP) $(TARGET_PATH)/*.asm
	@rm -f $(OBJS)

.PHONY : clean all

