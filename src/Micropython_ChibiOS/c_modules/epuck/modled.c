
// Include required definitions first.
#include "py/obj.h"
#include "py/runtime.h"
#include "py/builtin.h"
#include "py/mphal.h"
#include "modepuck.h"

#ifdef CHIBIOS
#include "ch.h"
#include "hal.h"
#include "leds.h"
#endif	/* CHIBIOS */

STATIC mp_obj_t epuck_led_set(mp_obj_t led, mp_obj_t value) {

    set_led(mp_obj_get_int(led), mp_obj_get_int(value));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(epuck_led_set_obj, epuck_led_set);

STATIC mp_obj_t epuck_led_clear(void) {

    clear_leds();

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(epuck_led_clear_obj, epuck_led_clear);

STATIC mp_obj_t epuck_led_set_body(mp_obj_t value) {

    set_body_led(mp_obj_get_int(value)); 

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(epuck_led_set_body_obj, epuck_led_set_body);

STATIC mp_obj_t epuck_led_set_front_led(mp_obj_t value) {

    set_front_led(mp_obj_get_int(value)); 

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(epuck_led_set_front_led_obj, epuck_led_set_front_led);

STATIC mp_obj_t epuck_led_set_rgb(mp_obj_t led, mp_obj_t values) {

    size_t len;
    mp_obj_t *val;

    mp_obj_get_array(values, &len, &val);
    if (len < 3 || len > 3) {
        mp_hal_stdout_tx_str("The array of RGB values should be of size 3\r\n");
        return mp_const_none;
    }

    set_rgb_led(mp_obj_get_int(led), mp_obj_get_int(val[0]), mp_obj_get_int(val[1]), mp_obj_get_int(val[2]));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(epuck_led_set_rgb_obj, epuck_led_set_rgb);


STATIC const mp_rom_map_elem_t epuck_led_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_led) },
    { MP_ROM_QSTR(MP_QSTR_set), MP_ROM_PTR(&epuck_led_set_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&epuck_led_clear_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_body), MP_ROM_PTR(&epuck_led_set_body_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_front_led), MP_ROM_PTR(&epuck_led_set_front_led_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_rgb), MP_ROM_PTR(&epuck_led_set_rgb_obj) },
    { MP_ROM_QSTR(MP_QSTR_LED1), MP_ROM_INT(LED1) },
    { MP_ROM_QSTR(MP_QSTR_RGB_LED2), MP_ROM_INT(LED2) },
    { MP_ROM_QSTR(MP_QSTR_LED3), MP_ROM_INT(LED3) },
    { MP_ROM_QSTR(MP_QSTR_RGB_LED4), MP_ROM_INT(LED4) },
    { MP_ROM_QSTR(MP_QSTR_LED5), MP_ROM_INT(LED5) },
    { MP_ROM_QSTR(MP_QSTR_RGB_LED6), MP_ROM_INT(LED6) },
    { MP_ROM_QSTR(MP_QSTR_LED7), MP_ROM_INT(LED7) },
    { MP_ROM_QSTR(MP_QSTR_RGB_LED8), MP_ROM_INT(LED8) },
    { MP_ROM_QSTR(MP_QSTR_ON), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_OFF), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_MAX_RGB), MP_ROM_INT(100) },
};
STATIC MP_DEFINE_CONST_DICT(epuck_led_module_globals, epuck_led_module_globals_table);

// Define module object.
const mp_obj_module_t epuck_led_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&epuck_led_module_globals,
};


/*! \brief Turn on/off the specified LED
 *
 * The e-puck2 has 4 red LEDs placed on front, right, back and left; these LEDs are directly controllable from the main processor (F407).
 * There are also 4 RGB LEDs placed at 45, 135, 225, 315 degrees; these LEDs are connected to the ESP32 and can be controlled thorugh SPI.
 * With this function, you can change the state of the 4 red LEDs, not the RGB LEDs.
 * \param led_number: LED1, LED3, LED5 or LED7 (LED1 is the front led, then continue clockwise)
 * \param value 0 (off), 1 (on) otherwise toggle the state
 * \warning if led_number is other than LED1-LED7, all leds are set to the indicated value.
 */
void set_led(led_name_t led_number, unsigned int value);

/*! \brief Turn off all the LEDs around the robot
 *
 * The e-puck2 has 4 red LEDs and 4 RGB LEDs placed on top of it. This function turn them all off.
 * \warning this function doesn't turn off "body LED" and "front LED".
 */
void clear_leds(void);

void set_body_led(unsigned int value); // value (0=off 1=on higher=inverse)
void set_front_led(unsigned int value); //value (0=off 1=on higher=inverse)

void get_all_rgb_state(uint8_t* values);
void toggle_rgb_led(rgb_led_name_t led_number, color_led_name_t led, uint8_t intensity);
void set_rgb_led(rgb_led_name_t led_number, uint8_t red_val, uint8_t green_val, uint8_t blue_val);