MPTOP ?= ../

include $(MPTOP)/py/mkenv.mk

all:lib

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

# include py core make definitions
include $(TOP)/py/py.mk

CROSS_COMPILE ?= arm-none-eabi-

INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD)
INC += -Ipython_flash_code/

CFLAGS_CORTEX_M7 = -mthumb -mtune=cortex-m4 -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv5-sp-d16 -fsingle-precision-constant -Wdouble-promotion
CFLAGS = $(INC) -Wall -Werror -std=c99 -nostdlib $(CFLAGS_CORTEX_M7) $(COPT)

CFLAGS += -Os -DNDEBUG -ggdb
CFLAGS += -fdata-sections -ffunction-sections

LIBS =

FROZEN_MANIFEST = manifest.py

MPY_CROSS_FLAGS =

SRC_C = \
	mpport.c \
	lib/utils/printf.c \
	lib/utils/stdout_helpers.c \
	lib/utils/pyexec.c \
	lib/libc/string0.c \
	lib/mp-readline/readline.c \
	lib/utils/interrupt_char.c \
	$(BUILD)/py_flash.py.c \

ifneq ($(FROZEN_MANIFEST),)
	SRC_C +=  $(BUILD)/frozen_content.c
endif

OBJ = $(PY_CORE_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o))

CLEAN_EXTRA = ./libmicropython.a

$(BUILD)/py_flash.py.c : python_flash_code/py_flash.py
	$(ECHO) "MISC freezing python code"
	$(Q)$(PYTHON) ./python2cArray.py $< > $@

include $(TOP)/py/mkrules.mk
