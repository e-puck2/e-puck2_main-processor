
// Include required definitions first.
#include "py/obj.h"
#include "py/runtime.h"
#include "py/builtin.h"
#include "modexample.h"

#ifdef CHIBIOS
#include "ch.h"
#include "hal.h"
#endif	/* CHIBIOS */

// This is the function which will be called from Python as example.add_ints(a, b).
STATIC mp_obj_t example_add(mp_obj_t a_obj, mp_obj_t b_obj) {
    // Extract the ints from the micropython input objects
    float a = mp_obj_get_float(a_obj);
    float b = mp_obj_get_float(b_obj);

    // Calculate the addition and convert to MicroPython object.
    return mp_obj_new_float(a + b);
}
// Define a Python reference to the function above
// MP_DEFINE_CONST_FUN_OBJ_2 means it takes two args as input
STATIC MP_DEFINE_CONST_FUN_OBJ_2(example_add_obj, example_add);


// This is the function which will be called from Python as example.add_ints(a, b).
STATIC mp_obj_t example_get_time(size_t n_args, const mp_obj_t *args) {

    if (n_args == 0 || args[0] == mp_const_none) {
        
        return mp_obj_new_int(chVTGetSystemTime());
    } else if (n_args == 1) {

        return mp_obj_new_int(chVTGetSystemTime() - mp_obj_get_int(args[0]));
    }else{
        mp_obj_t tuple[3] = {
            mp_obj_new_int(chVTGetSystemTime()),
            args[0],
            args[1],
        };
        return mp_obj_new_tuple(3, tuple);
    }
}
// Define a Python reference to the function above
// MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN means it takes beetween 0 and 2 args as input in this case
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(example_get_time_obj, 0, 2, example_get_time);

// Define all properties of the example module.
// Table entries are key/value pairs of the attribute name (a string)
// and the MicroPython object reference.
// All identifiers and strings are written as MP_QSTR_xxx and will be
// optimized to word-sized integers by the build system (interned strings).
STATIC const mp_rom_map_elem_t example_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_example) },
    { MP_ROM_QSTR(MP_QSTR_add), MP_ROM_PTR(&example_add_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_time), MP_ROM_PTR(&example_get_time_obj) },
};
STATIC MP_DEFINE_CONST_DICT(example_module_globals, example_module_globals_table);

// Define module object.
const mp_obj_module_t example_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&example_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_example, example_user_cmodule, MODULE_EXAMPLE_ENABLED);