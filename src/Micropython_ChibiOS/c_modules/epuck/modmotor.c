
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
#endif	/* CHIBIOS */

STATIC mp_obj_t epuck_motor_left_set_speed(mp_obj_t speed) {

    left_motor_set_speed(mp_obj_get_int(speed));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(epuck_motor_left_set_speed_obj, epuck_motor_left_set_speed);

STATIC mp_obj_t epuck_motor_right_set_speed(mp_obj_t speed) {

    right_motor_set_speed(mp_obj_get_int(speed));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(epuck_motor_right_set_speed_obj, epuck_motor_right_set_speed);

STATIC mp_obj_t epuck_motor_left_pos(size_t n_args, const mp_obj_t *args) {

    if (n_args == 0 || args[0] == mp_const_none){

        return mp_obj_new_int(left_motor_get_pos());

    }else if(n_args == 1){

        left_motor_set_pos(mp_obj_get_int(args[0]));

    }else{
        mp_hal_stdout_tx_str("Must give 0 or 1 arg\r\n");
    }

    return mp_const_none;    
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(epuck_motor_left_pos_obj, 0, 1, epuck_motor_left_pos);

STATIC mp_obj_t epuck_motor_right_pos(size_t n_args, const mp_obj_t *args) {

    if (n_args == 0 || args[0] == mp_const_none){

        return mp_obj_new_int(right_motor_get_pos());

    }else if(n_args == 1){

        right_motor_set_pos(mp_obj_get_int(args[0]));

    }else{
        mp_hal_stdout_tx_str("Must give 0 or 1 arg\r\n");
    }

    return mp_const_none;    
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(epuck_motor_right_pos_obj, 0, 1, epuck_motor_right_pos);


STATIC const mp_rom_map_elem_t epuck_motor_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_motor) },
    { MP_ROM_QSTR(MP_QSTR_left_set_speed), MP_ROM_PTR(&epuck_motor_left_set_speed_obj) },
    { MP_ROM_QSTR(MP_QSTR_right_set_speed), MP_ROM_PTR(&epuck_motor_right_set_speed_obj) },
    { MP_ROM_QSTR(MP_QSTR_left_pos), MP_ROM_PTR(&epuck_motor_left_pos_obj) },
    { MP_ROM_QSTR(MP_QSTR_right_pos), MP_ROM_PTR(&epuck_motor_right_pos_obj) },
};
STATIC MP_DEFINE_CONST_DICT(epuck_motor_module_globals, epuck_motor_module_globals_table);

// Define module object.
const mp_obj_module_t epuck_motor_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&epuck_motor_module_globals,
};