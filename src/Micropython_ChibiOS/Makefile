MPTOP ?= ../

include $(MPTOP)/py/mkenv.mk

all:
#tests if mpy-cross exists. If not, compiles it
ifeq ($(wildcard $(TOP)/mpy-cross/mpy-cross),)
	$(MAKE) -C $(TOP)/mpy-cross/
endif
	$(MAKE) lib

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

USER_C_MODULES = c_modules
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

# yes or no to compile and include or not the frozen files described by manifest.py
FROZEN_PYTHON ?= no
# yes or no to include or not the python script python_flash_code/py_flash.py in the flash
# needs to have correctly configured the .ld file
PYTHON_FLASH_CODE ?= no

MPY_CROSS_FLAGS =

SRC_C = \
	mpport.c \
	lib/utils/printf.c \
	lib/utils/stdout_helpers.c \
	lib/utils/pyexec.c \
	lib/libc/string0.c \
	lib/mp-readline/readline.c \
	lib/utils/interrupt_char.c


ifeq ($(FROZEN_PYTHON),yes)
	FROZEN_MANIFEST = manifest.py

	SRC_C += $(BUILD)/frozen_content.c

	CFLAGS += -DMICROPY_MODULE_FROZEN_MPY
	CFLAGS += -DMICROPY_QSTR_EXTRA_POOL=mp_qstr_frozen_const_pool
else
	FROZEN_MANIFEST =
endif

ifeq ($(PYTHON_FLASH_CODE),yes)
	SRC_C += $(BUILD)/py_flash.py.c

	CFLAGS += -DPYTHON_FLASH_CODE
endif

OBJ = $(PY_CORE_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o))

CLEAN_EXTRA = ./libmicropython.a

mpy-cross :
	$(MAKE) -C $(TOP)/mpy-cross/

ifeq ($(PYTHON_FLASH_CODE),yes)
$(BUILD)/py_flash.py.c : python_flash_code/py_flash.py
	$(ECHO) "MISC freezing python code"
	$(Q)$(PYTHON) ./python2cArray.py $< > $@
endif

include $(TOP)/py/mkrules.mk
