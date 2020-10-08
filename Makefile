SRC_DIR = src
BUILD_DIR = build
SRCS = $(wildcard $(SRC_DIR)/*.c)
INCLUDES = include include/system
TARGET = main
LDFILE = bootloader_stm32l432xx.ld
CPU = cortex-m4
PREFIX = arm-none-eabi-

OBJECTS = ${SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o}
DEPENDS = $(OBJECTS:.o=.d)

CC = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy

CFLAGS = -mcpu=$(CPU) -mthumb -Wall -std=c11 -Og -ffunction-sections -fdata-sections -g
LDFLAGS = -mcpu=$(CPU) -mthumb -specs=nano.specs -Wl,-Map=$(BUILD_DIR)/$(TARGET).map -Wl,--gc-sections -T$(LDFILE)

CFLAGS += $(foreach i,$(INCLUDES),-I$(i))

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) | $(BUILD_DIR)
	$(CC) $(LDFLAGS) $^ -o $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)
	$(OBJCOPY) -O binary -S $< $@

-include ${DEPENDS}

$(BUILD_DIR):
	mkdir $@

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
