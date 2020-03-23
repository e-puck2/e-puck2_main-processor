
MPTOP_CHIBIOS ?= $(MPTOP)/Micropython_ChibiOS

ALLDEFS +=	-DNO_QSTR

# Include directories
#Need to add to USE_OPT and not ALLINC if we want to use -isystem option
#otherwisee it is bypassed by -I in the rules.mk of ChibiOS
USE_OPT += 	-isystem $(MPTOP) \
			-isystem $(MPTOP_CHIBIOS) \
			-isystem $(MPTOP_CHIBIOS)/python_flash_code \
# 			$(MPTOP)/ports/ChibiOS/build \
# 			$(BUILD)/genhdr

ALLCSRC	+=	$(MPTOP_CHIBIOS)/flash/mp_flash.c \
			$(MPTOP_CHIBIOS)/mp_platform.c

ALLINC	+=	$(MPTOP_CHIBIOS)/ \
			$(MPTOP_CHIBIOS)/mp_flash/

preall: 
	$(MAKE) -C $(MPTOP_CHIBIOS)/ MPTOP=$(MPTOP_FOR_MP_MAKEFILE);
	$(MAKE) all

CLEAN_RULE_HOOK:
	$(MAKE) -C $(MPTOP_CHIBIOS)/ clean MPTOP=$(MPTOP_FOR_MP_MAKEFILE);
