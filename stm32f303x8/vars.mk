DEV_FAMILY = STM32F3xx
DEV_DIR = stm32f303x8

DEV_INCLUDES = \
-I $(DEV_DIR)/CMSIS/Device/ST/$(DEV_FAMILY)/Include \
-I $(DEV_DIR)/CMSIS/Include \
-I $(DEV_DIR)/inc

DEV_C_SOURCES := $(wildcard $(DEV_DIR)/src/*.c)
DEV_C_SOURCES += $(DEV_DIR)/CMSIS/Device/ST/$(DEV_FAMILY)/Source/system_stm32f3xx.c \

DEV_DEFS = -D STM32F303x8

LD_SCRIPT = $(DEV_DIR)/bootloader.ld
STARTUP = $(DEV_DIR)/startup.s

CPU = -mcpu=cortex-m4
FPU = -mfpu=fpv4-sp-d16
FLOAT-ABI = -mfloat-abi=hard
