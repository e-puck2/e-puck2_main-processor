
// Include required definitions first.
#include "py/obj.h"
#include "py/runtime.h"
#include "py/builtin.h"
#include "py/mphal.h"
#include "modepuck.h"

#ifdef CHIBIOS
#include "ch.h"
#include "hal.h"
#include "motors.h"
#include "leds.h"
#endif	/* CHIBIOS */


STATIC const mp_rom_map_elem_t epuck_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_epuck) },
    { MP_ROM_QSTR(MP_QSTR_led), MP_ROM_PTR(&epuck_led_cmodule) },
    { MP_ROM_QSTR(MP_QSTR_motor), MP_ROM_PTR(&epuck_motor_cmodule) },

};
STATIC MP_DEFINE_CONST_DICT(epuck_module_globals, epuck_module_globals_table);

// Define module object.
const mp_obj_module_t epuck_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&epuck_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_epuck, epuck_cmodule, MODULE_EPUCK_ENABLED);