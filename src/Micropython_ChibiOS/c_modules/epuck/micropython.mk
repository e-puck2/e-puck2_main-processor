EPUCK_MOD_DIR := $(USERMOD_DIR)

# Add all C files to SRC_USERMOD to generate the QSTR
SRC_USERMOD += 	$(EPUCK_MOD_DIR)/modepuck.c \
				$(EPUCK_MOD_DIR)/modled.c \
				$(EPUCK_MOD_DIR)/modmotor.c
# Add all C files to ALLCSRC to compile with ChibiOS
ALLCSRC += 	$(EPUCK_MOD_DIR)/modepuck.c \
			$(EPUCK_MOD_DIR)/modled.c \
			$(EPUCK_MOD_DIR)/modmotor.c


ALLINC += $(EPUCK_MOD_DIR)
