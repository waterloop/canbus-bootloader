include ./stm32f303x8/vars.mk

TARGET = main
BUILD_DIR = build

DEBUG = 1
OPT = -Os

CC = arm-none-eabi-gcc
AS = arm-none-eabi-gcc -x assembler-with-cpp
CP = arm-none-eabi-objcopy
SZ = arm-none-eabi-size
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

C_SOURCES = $(DEV_C_SOURCES)
C_SOURCES += $(wildcard ./src/*.c)


ASM_SOURCES = $(STARTUP)

C_INCLUDES = \
-I inc \
$(DEV_INCLUDES)

MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)
LIBS = -lc -lm -lnosys

C_FLAGS = $(C_INCLUDES) $(DEV_DEFS) $(MCU) $(OPT) -fdata-sections -ffunction-sections -Wall
C_FLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

ifeq ($(DEBUG), 1)
	C_FLAGS += -ggdb
endif

AS_FLAGS = $(MCU) $(OPT) -Wall -fdata-sections -ffunction-sections

LD_FLAGS = $(MCU) -specs=nano.specs -T $(LD_SCRIPT) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections


OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) -c $(C_FLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@
	@echo ""

$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	$(AS) -c $(AS_FLAGS) $< -o $@
	@echo ""

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)
	$(CC) $(OBJECTS) $(LD_FLAGS) -o $@
	@echo ""
	$(SZ) $@
	@echo ""

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@
	
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@	
	
$(BUILD_DIR):
	mkdir $@		

clean:
	rm -rf $(BUILD_DIR)

flash:
	st-flash write $(BUILD_DIR)/$(TARGET).bin 0x08000000
	st-flash reset

-include $(wildcard $(BUILD_DIR)/*.d)
