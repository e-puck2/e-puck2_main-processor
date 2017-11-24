
INCDIR += ./ChibiOS_ext/os/hal/include

INCDIR += ./ChibiOS_ext/os/hal/ports/STM32/STM32F4xx

INCDIR += src/


CSRC += ./ChibiOS_ext/os/hal/ports/STM32/STM32F4xx/dcmi_lld.c
CSRC += ./ChibiOS_ext/os/hal/src/dcmi.c
CSRC += ./src/camera/dcmi_camera.c
CSRC += ./src/camera/po8030.c
CSRC += ./src/cmd.c
CSRC += ./src/flash/flash.c
CSRC += ./src/i2c_bus.c
CSRC += ./src/leds.c
CSRC += ./src/main.c
CSRC += ./src/memory_protection.c
CSRC += ./src/motors.c
CSRC += ./src/panic.c
CSRC += ./src/usbcfg.c
CSRC += ./src/utility.c
CSRC += src/chibios-syscalls/malloc_lock.c
CSRC += src/chibios-syscalls/newlib_syscalls.c
CSRC += src/msgbus/examples/chibios/port.c
CSRC += ./src/config_flash_storage.c
CSRC += src/cmp/cmp.c
CSRC += src/cmp_mem_access/cmp_mem_access.c
CSRC += src/crc/crc16.c
CSRC += src/crc/crc32.c
CSRC += src/msgbus/messagebus.c
CSRC += src/parameter/parameter.c
CSRC += src/parameter/parameter_msgpack.c
CSRC += src/parameter/parameter_print.c
