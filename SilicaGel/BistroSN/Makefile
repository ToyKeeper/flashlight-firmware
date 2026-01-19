MCU := SN8F5701
LAYOUT := CONVOY_7135
FLASH_SIZE := 4096
IRAM_SIZE := 256
XRAM_SIZE := 0
XRAM_LOC := 0
SRC_DIRS := ./src
BUILD_DIR := ./build
TARGET_NAME := $(shell echo $(MCU)_$(LAYOUT) | tr A-Z a-z)
TARGET_DIR := $(BUILD_DIR)/$(TARGET_NAME)
BIN := bistro_$(TARGET_NAME)
PORT := 

CC := sdcc
AS := sdas8051
MAKEBIN := makebin
FLASH := sn8flash
AWK := "$(shell awk 'BEGIN {print "awk"}' || busybox awk 'BEGIN {print "busybox awk"}')"

SRCS := \
  $(shell find $(SRC_DIRS) -name "*.c") \
  $(shell find $(SRC_DIRS) -name "*.asm")
OBJS := $(SRCS:%=$(TARGET_DIR)/%.rel)
DEPS := $(OBJS:.rel=.d)

INC_DIRS := $(SRC_DIRS)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))
DEFINE_FLAGS := \
  -DMCU_$(MCU) -DMCU_DEFINED \
  -DLAYOUT_$(LAYOUT) -DLAYOUT_DEFINED

CFLAGS := $(INC_FLAGS) $(DEFINE_FLAGS) -mmcs51 --opt-code-size --no-xinit-opt --model-small \
  --xram-size $(XRAM_SIZE) --xram-loc $(XRAM_LOC) --iram-size $(IRAM_SIZE) \
  --code-size $(FLASH_SIZE) --out-fmt-ihx --std-sdcc11 -MMD --Werror
ASFLAGS := -los $(INC_FLAGS)
LDFLAGS :=

.PHONY: all
all: $(TARGET_DIR)/$(BIN).bin $(TARGET_DIR)/$(BIN).hex size

$(TARGET_DIR)/$(BIN).bin: $(TARGET_DIR)/$(BIN).hex
	$(MAKEBIN) -s $(FLASH_SIZE) $< $@

$(TARGET_DIR)/$(BIN).hex: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@

$(TARGET_DIR)/%.c.rel: %.c Makefile
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(EXTRA_FLAGS) -c $< -o $@

$(TARGET_DIR)/%.asm.rel: %.asm Makefile
	mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $@ $<

.PHONY: size
size: $(TARGET_DIR)/$(BIN).hex
	@echo "------------------"
	@echo "FLASH: $(shell $(AWK) '$$1 == "ROM/EPROM/FLASH"      {print $$4}' $(TARGET_DIR)/$(BIN).mem) bytes"
	@echo "IRAM:  $(shell $(AWK) '$$1 == "Stack"           {print 248-$$10}' $(TARGET_DIR)/$(BIN).mem) bytes"
	@echo "XRAM:  $(shell $(AWK) '$$1 == "EXTERNAL" {print $(XRAM_LOC)+$$5}' $(TARGET_DIR)/$(BIN).mem) bytes"
	@echo "------------------"

.PHONY: flash
flash: all
ifndef PORT
	$(error Variable PORT is not provided. Please use "make flash PORT=<port>")
endif
	$(FLASH) --port $(PORT) write --file $(TARGET_DIR)/$(BIN).hex

.PHONY: clean
clean:
	rm -fr $(BUILD_DIR)

-include $(DEPS)
