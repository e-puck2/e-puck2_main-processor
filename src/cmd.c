#include <math.h>
#include <string.h>
#include <stdio.h>

#include "hal.h"
#include "test.h"
#include "chprintf.h"
#include "shell.h"
#include "usbcfg.h"
#include "chtm.h"
#include "common/types.h"
#include "vm/natives.h"
#include "audio/audio_thread.h"
#include "audio/microphone.h"
#include "camera/po8030.h"
#include "camera/dcmi_camera.h"
#include "sensors/battery_level.h"
#include "config_flash_storage.h"
#include "leds.h"
#include "main.h"
#include "motors.h"

#define TEST_WA_SIZE        THD_WORKING_AREA_SIZE(256)
#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)

static void cmd_mem(BaseSequentialStream *chp, int argc, char *argv[])
{
    size_t n, size;

    (void)argv;
    if (argc > 0) {
        chprintf(chp, "Usage: mem\r\n");
        return;
    }
    n = chHeapStatus(NULL, &size);
    chprintf(chp, "core free memory : %u bytes\r\n", chCoreGetStatusX());
    chprintf(chp, "heap fragments     : %u\r\n", n);
    chprintf(chp, "heap free total    : %u bytes\r\n", size);
}

static void cmd_threads(BaseSequentialStream *chp, int argc, char *argv[])
{
    static const char *states[] = {CH_STATE_NAMES};
    thread_t *tp;

    (void)argv;
    if (argc > 0) {
        chprintf(chp, "Usage: threads\r\n");
        return;
    }
    chprintf(chp, "        addr        stack prio refs         state\r\n");
    tp = chRegFirstThread();
    do {
        chprintf(chp, "%08lx %08lx %4lu %4lu %9s\r\n",
                 (uint32_t)tp, (uint32_t)tp->p_ctx.r13,
                 (uint32_t)tp->p_prio, (uint32_t)(tp->p_refs - 1),
                 states[tp->p_state]);
        tp = chRegNextThread(tp);
    } while (tp != NULL);
}

static void cmd_test(BaseSequentialStream *chp, int argc, char *argv[])
{
    thread_t *tp;

    (void)argv;
    if (argc > 0) {
        chprintf(chp, "Usage: test\r\n");
        return;
    }
    tp = chThdCreateFromHeap(NULL, TEST_WA_SIZE, chThdGetPriorityX(),
                             TestThread, chp);
    if (tp == NULL) {
        chprintf(chp, "out of memory\r\n");
        return;
    }
    chThdWait(tp);
}

static void cmd_readclock(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    chprintf(chp, "SYSCLK: %i \n HCLK: %i \n PCLK1  %i \n PCLK2 %i \n",
             STM32_SYSCLK, STM32_HCLK, STM32_PCLK1, STM32_PCLK2);
}


extern sint16 aseba_sqrt(sint16 num);

static void cmd_sqrt(BaseSequentialStream *chp, int argc, char *argv[])
{
    uint16_t input, result;
    float x;
    time_measurement_t tmp;
    chTMObjectInit(&tmp);

    if (argc != 2) {
        chprintf(chp,
                 "Usage: sqrt mode int\r\nModes: a (aseba), b (math), c (assembler) is default mode\r\n");
    } else {
        input = (uint16_t) atoi(argv[1]);

        if (!strcmp(argv[0], "a")) {
            chSysLock();
            chTMStartMeasurementX(&tmp);
            result = aseba_sqrt(input);
            chTMStopMeasurementX(&tmp);
            chSysUnlock();
        } else if (!strcmp(argv[0], "b")) {
            chSysLock();
            chTMStartMeasurementX(&tmp);
            result = sqrtf(input);
            chTMStopMeasurementX(&tmp);
            chSysUnlock();
        } else {
            chSysLock();
            chTMStartMeasurementX(&tmp);
            x = (float) input;
            __asm__ volatile (
                "vsqrt.f32 %[var], %[var]"
                : [var] "+t" (x)
                );
            result = (uint16_t) x;
            chTMStopMeasurementX(&tmp);
            chSysUnlock();
        }

        chprintf(chp, "sqrt(%u) = %u \r\n", input, result);
        chprintf(chp, "time: %u \r\n", tmp.last);
    }
}

extern sint16 aseba_atan2(sint16 y, sint16 x);

static void cmd_atan2(BaseSequentialStream *chp, int argc, char *argv[])
{
    int16_t a, b, result;
    time_measurement_t tmp;
    chTMObjectInit(&tmp);

    if (argc != 3) {
        chprintf(chp, "Usage: atan2 mode a b\r\nModes: a (aseba), b (math) is default mode\r\n");
    } else {
        a = (int16_t) atoi(argv[1]);
        b = (int16_t) atoi(argv[2]);

        if (!strcmp(argv[0], "a")) {
            chSysLock();
            chTMStartMeasurementX(&tmp);
            result = aseba_atan2(a, b);
            chTMStopMeasurementX(&tmp);
            chSysUnlock();
        } else {
            chSysLock();
            chTMStartMeasurementX(&tmp);
            result = (int16_t)(atan2f(a, b) * 32768 / M_PI);
            chTMStopMeasurementX(&tmp);
            chSysUnlock();
        }


        chprintf(chp, "atan2(%d, %d) = %d \r\n", a, b, result);
        chprintf(chp, "time: %u \r\n", tmp.last);
    }
}

static void tree_indent(BaseSequentialStream *out, int indent)
{
    int i;
    for (i = 0; i < indent; ++i) {
        chprintf(out, "  ");
    }
}

static void show_config_tree(BaseSequentialStream *out, parameter_namespace_t *ns, int indent)
{
    parameter_t *p;
    char string_buf[64];

    tree_indent(out, indent);
    chprintf(out, "%s:\r\n", ns->id);

    for (p = ns->parameter_list; p != NULL; p = p->next) {
        tree_indent(out, indent + 1);
        if (parameter_defined(p)) {
            switch (p->type) {
                case _PARAM_TYPE_SCALAR:
                    chprintf(out, "%s: %f\r\n", p->id, parameter_scalar_get(p));
                    break;

                case _PARAM_TYPE_INTEGER:
                    chprintf(out, "%s: %d\r\n", p->id, parameter_integer_get(p));
                    break;

                case _PARAM_TYPE_BOOLEAN:
                    chprintf(out, "%s: %s\r\n", p->id, parameter_boolean_get(p) ? "true" : "false");
                    break;

                case _PARAM_TYPE_STRING:
                    parameter_string_get(p, string_buf, sizeof(string_buf));
                    chprintf(out, "%s: %s\r\n", p->id, string_buf);
                    break;

                default:
                    chprintf(out, "%s: unknown type %d\r\n", p->id, p->type);
                    break;
            }
        } else {
            chprintf(out, "%s: [not set]\r\n", p->id);
        }
    }

    if (ns->subspaces) {
        show_config_tree(out, ns->subspaces, indent + 1);
    }

    if (ns->next) {
        show_config_tree(out, ns->next, indent);
    }
}

static void cmd_config_tree(BaseSequentialStream *chp, int argc, char **argv)
{
    parameter_namespace_t *ns;
    if (argc != 1) {
        ns = &parameter_root;
    } else {
        ns = parameter_namespace_find(&parameter_root, argv[0]);
        if (ns == NULL) {
            chprintf(chp, "Cannot find subtree.\r\n");
            return;
        }
    }

    show_config_tree(chp, ns, 0);
}

static void cmd_config_set(BaseSequentialStream *chp, int argc, char **argv)
{
    parameter_t *param;
    int value_i;

    if (argc != 2) {
        chprintf(chp, "Usage: config_set /parameter/url value.\r\n");
        return;
    }

    param = parameter_find(&parameter_root, argv[0]);

    if (param == NULL) {
        chprintf(chp, "Could not find parameter \"%s\"\r\n", argv[0]);
        return;
    }

    switch (param->type) {
        case _PARAM_TYPE_INTEGER:
            if (sscanf(argv[1], "%d", &value_i) == 1) {
                parameter_integer_set(param, value_i);
            } else {
                chprintf(chp, "Invalid value for integer parameter.\r\n");
            }
            break;

        case _PARAM_TYPE_BOOLEAN:
            if (!strcmp(argv[1], "true")) {
                parameter_boolean_set(param, true);
            } else if (!strcmp(argv[1], "false")) {
                parameter_boolean_set(param, false);
            } else {
                chprintf(chp, "Invalid value for boolean parameter, must be true or false.\r\n");
            }
            break;

        case _PARAM_TYPE_STRING:
            if (argc == 2) {
                parameter_string_set(param, argv[1]);
            } else {
                chprintf(chp, "Invalid value for string parameter, must not use spaces.\r\n");
            }
            break;

        default:
            chprintf(chp, "%s: unknown type %d\r\n", param->id, param->type);
            break;
    }
}

static void cmd_config_erase(BaseSequentialStream *chp, int argc, char **argv)
{
    (void) argc;
    (void) argv;
    (void) chp;
    extern uint8_t _config_start;

    config_erase(&_config_start);
}

static void cmd_config_save(BaseSequentialStream *chp, int argc, char **argv)
{
    (void) argc;
    (void) argv;
    extern uint8_t _config_start, _config_end;
    size_t len = (size_t)(&_config_end - &_config_start);
    bool success;

    // First write the config to flash
    config_save(&_config_start, len, &parameter_root);

    // Second try to read it back, see if we failed
    success = config_load(&parameter_root, &_config_start);

    if (success) {
        chprintf(chp, "OK.\r\n");
    } else {
        chprintf(chp, "Save failed.\r\n");
    }
}

static void cmd_config_load(BaseSequentialStream *chp, int argc, char **argv)
{
    (void) argc;
    (void) argv;
    extern uint8_t _config_start;
    bool success;

    success = config_load(&parameter_root, &_config_start);

    if (success) {
        chprintf(chp, "OK.\r\n");
    } else {
        chprintf(chp, "Load failed.\r\n");
    }
}

static void cmd_cam_set_brightness(BaseSequentialStream *chp, int argc, char *argv[])
{
    int8_t value, err;

    if (argc != 1) {
        chprintf(chp,
                 "Usage: cam_brightness value.\r\nDefault=0, max=127, min=-128.\r\n");
    } else {
        value = (int8_t) atoi(argv[0]);
        err = po8030_set_brightness(value);
        if(err != MSG_OK) {
            chprintf(chp, "Cannot write register (%d)\r\n", err);
        } else {
            chprintf(chp, "Register written correctly\r\n");
        }
    }
}

static void cmd_cam_set_contrast(BaseSequentialStream *chp, int argc, char *argv[])
{
    int8_t value, err;

    if (argc != 1) {
        chprintf(chp,
                 "Usage: cam_contrast value.\r\nDefault=64, max=255, min=0.\r\n");
    } else {
        value = (int8_t) atoi(argv[0]);
        err = po8030_set_contrast(value);
        if(err != MSG_OK) {
            chprintf(chp, "Cannot write register (%d)\r\n", err);
        } else {
            chprintf(chp, "Register written correctly\r\n");
        }
    }
}

static void cmd_cam_set_adv_conf_fmt(BaseSequentialStream *chp, int argc, char *argv[])
{
    uint16_t f;
    format_t fmt;

    if (argc != 1) {
        chprintf(chp,
                 "Usage: cam_adv_conf_fmt format\r\nformat: 0=color, 1=grey\r\n");
    } else {
        f = (uint8_t) atoi(argv[0]);

        if(f==0) {
            fmt = FORMAT_YCBYCR;
            chprintf(chp, "Registered color format\r\n");
        } else {
            fmt = FORMAT_YYYY;
            chprintf(chp, "Registered greyscale format\r\n");
        }

        po8030_save_current_format(fmt);
    }
}

static void cmd_cam_set_adv_conf_sub(BaseSequentialStream *chp, int argc, char *argv[])
{
    uint8_t sx, sy;
    subsampling_t subx, suby;

    if (argc != 2) {
        chprintf(chp,
                 "Usage: cam_adv_conf_sub subsampling_x subsampling_y\r\nsubsampling: 1, 2, 4\r\n");
    } else {
        sx = (uint8_t) atoi(argv[0]);
        sy = (uint8_t) atoi(argv[1]);

        if(sx == 1) {
            subx = SUBSAMPLING_X1;
            chprintf(chp, "Registered x subsampling x1\r\n");
        } else if(sx == 2) {
            subx = SUBSAMPLING_X2;
            chprintf(chp, "Registered x subsampling x2\r\n");
        } else {
            subx = SUBSAMPLING_X4;
            chprintf(chp, "Registered x subsampling x4\r\n");
        }

        if(sy == 1) {
            suby = SUBSAMPLING_X1;
            chprintf(chp, "Registered y subsampling x1\r\n");
        } else if(sy == 2) {
            suby = SUBSAMPLING_X2;
            chprintf(chp, "Registered y subsampling x2\r\n");
        } else {
            suby = SUBSAMPLING_X4;
            chprintf(chp, "Registered y subsampling x4\r\n");
        }

        po8030_save_current_subsampling(subx, suby);
    }
}

static void cmd_cam_set_adv_conf_win(BaseSequentialStream *chp, int argc, char *argv[])
{
    int8_t err;
    uint16_t x1, y1, width, height;
    subsampling_t subx, suby;
    format_t fmt;

    if (argc != 4) {
        chprintf(chp,
                 "Usage: cam_adv_conf x1 y1 width height\r\n");
    } else {
        x1 = (uint16_t) atoi(argv[0]);
        y1 = (uint16_t) atoi(argv[1]);
        width = (uint16_t) atoi(argv[2]);
        height = (uint16_t) atoi(argv[3]);
        fmt = po8030_get_saved_format();
        subx = po8030_get_saved_subsampling_x();
        suby = po8030_get_saved_subsampling_y();
				
        err = po8030_advanced_config(fmt, x1, y1, width, height, subx, suby);
        if(err != MSG_OK) {
            chprintf(chp, "Cannot set configuration (%d)\r\n", err);
        } else {
            chprintf(chp, "Configuration set correctly\r\n");
        }
    }
}

static void cmd_cam_set_mirror(BaseSequentialStream *chp, int argc, char *argv[])
{
    int8_t err;
    uint8_t v, h;

    if (argc != 2) {
        chprintf(chp,
                 "Usage: cam_mirror vertical_en horizontal_en\r\n1=enabled, 0=disabled\r\n");
    } else {
        v = (uint8_t) atoi(argv[0]);
        h = (uint8_t) atoi(argv[1]);

        err = po8030_set_mirror(v, h);
        if(err != MSG_OK) {
            chprintf(chp, "Cannot set mirroring (%d)\r\n", err);
        } else {
            chprintf(chp, "Mirroring set correctly\r\n");
        }
    }
}

static void cmd_cam_set_gain(BaseSequentialStream *chp, int argc, char *argv[])
{
    int8_t err;
    uint8_t r, g, b;

    if (argc != 3) {
        chprintf(chp,
                 "Usage: cam_gain red_gain green_gain blue_gain\r\nThis command disable auto white balance.\r\nDefault: r=94, g=64, b=93\r\n");
    } else {
        r = (uint8_t) atoi(argv[0]);
        g = (uint8_t) atoi(argv[1]);
        b = (uint8_t) atoi(argv[2]);

        err = po8030_set_rgb_gain(r, g, b);
        if(err != MSG_OK) {
            chprintf(chp, "Cannot set gain (%d)\r\n", err);
        } else {
            chprintf(chp, "Gain set correctly\r\n");
        }
    }
}

static void cmd_cam_set_awb(BaseSequentialStream *chp, int argc, char *argv[])
{
    int8_t err;
    uint8_t awb;

    if (argc != 1) {
        chprintf(chp,
                 "Usage: cam_awb awb_en.\r\n1=enabled, 0=disabled\r\n");
    } else {
        awb = (uint8_t) atoi(argv[0]);

        err = po8030_set_awb(awb);
        if(err != MSG_OK) {
            chprintf(chp, "Cannot set white balance (%d)\r\n", err);
        } else {
            chprintf(chp, "White balance set correctly\r\n");
        }
    }
}

static void cmd_cam_set_ae(BaseSequentialStream *chp, int argc, char *argv[])
{
    int8_t err;
    uint8_t ae;

    if (argc != 1) {
        chprintf(chp,
                 "Usage: cam_ae ae_en.\r\n1=enabled, 0=disabled\r\n");
    } else {
        ae = (uint8_t) atoi(argv[0]);

        err = po8030_set_ae(ae);
        if(err != MSG_OK) {
            chprintf(chp, "Cannot set auto exposure (%d)\r\n", err);
        } else {
            chprintf(chp, "Auto exposure set correctly\r\n");
        }
    }
}

static void cmd_cam_set_exposure(BaseSequentialStream *chp, int argc, char *argv[])
{
    int8_t err;
    uint16_t integral;
    uint8_t fractional;

    if (argc != 2) {
        chprintf(chp,
                 "Usage: cam_exposure integral fractional\r\nUnit is line time; total integration time = (integral + fractional/256) line time.\r\nDefault: integral=128, fractional=0\r\n");
    } else {
        integral = (uint16_t) atoi(argv[0]);
        fractional = (uint8_t) atoi(argv[1]);

        err = po8030_set_exposure(integral, fractional);
        if(err != MSG_OK) {
            chprintf(chp, "Cannot set exposure time (%d)\r\n", err);
        } else {
            chprintf(chp, "Exposure time set correctly\r\n");
        }
    }
}

static void cmd_cam_dcmi_prepare(BaseSequentialStream *chp, int argc, char **argv)
{
    uint32_t image_size = 0;

    if (argc != 1) {
        chprintf(chp,
                 "Usage: cam_dcmi_prepare capture_mode\r\ncapture_mode: 0=oneshot, 1=continuous\r\n");
    } else {
        capture_mode = (uint8_t) atoi(argv[0]);
        image_size = po8030_get_image_size();

        if(image_size > MAX_BUFF_SIZE) {
            chprintf(chp, "Cannot prepare dcmi, image size too big.\r\n");
            return;
        }

        if(image_size > (MAX_BUFF_SIZE/2)) {
            double_buffering = 0;
        } else {
            double_buffering = 1;
        }

        if(sample_buffer != NULL) {
            chprintf(chp, "Cannot prepare dcmi, buffer1 already allocated.\r\n");
            return;
        }
        sample_buffer = (uint8_t*)malloc(image_size);
        if(sample_buffer == NULL) {
            chprintf(chp, "Could not allocate buffer1\r\n");
            return;
        }

        if(capture_mode == CAPTURE_ONE_SHOT) {
            dcmi_prepare(&DCMID, &dcmicfg, image_size, (uint32_t*)sample_buffer, NULL);
            chprintf(chp, "DCMI prepared with single-buffering\r\n");
        } else {
            if(double_buffering == 0) {
                dcmi_prepare(&DCMID, &dcmicfg, image_size, (uint32_t*)sample_buffer, NULL);
                chprintf(chp, "DCMI prepared with single-buffering\r\n");
            } else {
                if(sample_buffer2 != NULL) {
                    chprintf(chp, "Cannot prepare dcmi, buffer2 already allocated.\r\n");
                    return;
                }
                sample_buffer2 = (uint8_t*)malloc(image_size);
                if(sample_buffer2 == NULL) {
                    chprintf(chp, "Could not allocate buffer2\r\n");
                    return;
                }
                dcmi_prepare(&DCMID, &dcmicfg, image_size, (uint32_t*)sample_buffer, (uint32_t*)sample_buffer2);
                chprintf(chp, "DCMI prepared with double-buffering\r\n");
            }
        }
    }
}

static void cmd_cam_dcmi_unprepare(BaseSequentialStream *chp, int argc, char **argv)
{
    (void) argc;
    (void) argv;

    dcmi_unprepare(&DCMID);

    if(sample_buffer != NULL) {
        free(sample_buffer);
        sample_buffer = NULL;
    }

    if(sample_buffer2 != NULL) {
        free(sample_buffer2);
        sample_buffer2 = NULL;
    }

    chprintf(chp, "DCMI released correctly\r\n");

}

static void cmd_cam_capture(BaseSequentialStream *chp, int argc, char **argv)
{
	(void) chp;
    (void) argc;
    (void) argv;

	if(capture_mode == CAPTURE_ONE_SHOT) {
		dcmi_start_one_shot(&DCMID);
	} else {
		dcmi_start_stream(&DCMID);
	}

}

static void cmd_cam_send(BaseSequentialStream *chp, int argc, char **argv)
{
    (void) argc;
    (void) argv;

	if(capture_mode == CAPTURE_ONE_SHOT) {
		chprintf(chp, "The image will be sent within 5 seconds\r\n");
		chThdSleepMilliseconds(5000);
		chnWrite((BaseSequentialStream *)&SDU1, sample_buffer, po8030_get_image_size());
	} else {
		if(dcmi_stop_stream(&DCMID) == MSG_OK) {
			if(double_buffering == 1) { // Send both images.
				chprintf(chp, "The 2 images will be sent within 5 seconds\r\n");
				chThdSleepMilliseconds(5000);
				chnWrite((BaseSequentialStream *)&SDU1, sample_buffer, po8030_get_image_size());
				chThdSleepMilliseconds(3000);
				chnWrite((BaseSequentialStream *)&SDU1, sample_buffer2, po8030_get_image_size());
			} else {
				chprintf(chp, "The image will be sent within 5 seconds\r\n");
				chThdSleepMilliseconds(5000);
				chnWrite((BaseSequentialStream *)&SDU1, sample_buffer, po8030_get_image_size());
			}
		} else {
			chprintf(chp, "DCMI stop stream error\r\n");
		}
	}
}

static void cmd_set_led(BaseSequentialStream *chp, int argc, char **argv)
{
    uint8_t led_num = 0;
	uint8_t led_value = 0;

    if (argc != 2) {
        chprintf(chp, "Usage: set_led led_num led_value\r\nled_num: 0-3=small leds, 4=body led, 5=front led\r\nvalue: 0=off, 1=on, 2=toggle\r\n");
    } else {
        led_num = (uint8_t) atoi(argv[0]);
        led_value = (uint8_t) atoi(argv[1]);
		
		if(led_num <= 3) {
			set_led(led_num, led_value);
		} else if(led_num == 4) {
			set_body_led(led_value);
		} else if(led_num == 5) {
			set_front_led(led_value);
		}
    }
}

static void cmd_set_speed(BaseSequentialStream *chp, int argc, char **argv)
{
    int16_t speed_left = 0;
	int16_t speed_right = 0;

    if (argc != 2) {
        chprintf(chp, "Usage: set_speed left right\r\nspeed: %d..%d\r\n", -MOTOR_SPEED_LIMIT, MOTOR_SPEED_LIMIT);
    } else {
        speed_left = (int16_t) atoi(argv[0]);
        speed_right = (int16_t) atoi(argv[1]);
		chprintf(chp, "lspeed=%d, rspeed=%d\r\n", speed_left, speed_right);
		right_motor_set_speed(speed_right);
		left_motor_set_speed(speed_left);
    }
}

static void cmd_get_battery(BaseSequentialStream *chp, int argc, char **argv)
{
	(void) chp;
    (void) argc;
    (void) argv;
    chprintf(chp, "Battery raw value = %d\r\n", get_battery_raw());
}

static void cmd_audio_play(BaseSequentialStream *chp, int argc, char *argv[])
{
    uint16_t freq;
    if (argc != 1) {
        chprintf(chp,
                 "Usage: audio_play freq\r\nfreq=100..20000 Hz\r\n");
    } else {
    	freq = (uint16_t) atoi(argv[0]);
        dac_play(freq);
    }
}

static void cmd_audio_stop(BaseSequentialStream *chp, int argc, char **argv)
{
	(void) chp;
    (void) argc;
    (void) argv;
    dac_stop();
}

static void cmd_volume(BaseSequentialStream *chp, int argc, char *argv[])
{
    uint8_t mic;
    if (argc != 1) {
        chprintf(chp, "Usage: volume mic_num.\r\nmic_num=0..3\r\n");
    } else {
        mic = (uint8_t) atoi(argv[0]);
        chprintf(chp, "%d\r\n", mic_get_volume(mic));
    }
}

const ShellCommand shell_commands[] = {
    {"mem", cmd_mem},
    {"threads", cmd_threads},
    {"test", cmd_test},
    {"clock", cmd_readclock},
    {"sqrt", cmd_sqrt},
    {"atan2", cmd_atan2},
    {"config_tree", cmd_config_tree},
    {"config_set", cmd_config_set},
    {"config_save", cmd_config_save},
    {"config_load", cmd_config_load},
    {"config_erase", cmd_config_erase},
    {"cam_brightness", cmd_cam_set_brightness},
    {"cam_contrast", cmd_cam_set_contrast},
    {"cam_adv_conf_fmt", cmd_cam_set_adv_conf_fmt},
    {"cam_adv_conf_sub", cmd_cam_set_adv_conf_sub},
    {"cam_adv_conf_win", cmd_cam_set_adv_conf_win},
    {"cam_mirror", cmd_cam_set_mirror},
    {"cam_gain", cmd_cam_set_gain},
    {"cam_awb", cmd_cam_set_awb},
    {"cam_ae", cmd_cam_set_ae},
    {"cam_exposure", cmd_cam_set_exposure},
    {"cam_dcmi_prepare", cmd_cam_dcmi_prepare},
    {"cam_dcmi_unprepare", cmd_cam_dcmi_unprepare},
	{"cam_capture", cmd_cam_capture},
	{"cam_send", cmd_cam_send},
	{"set_led", cmd_set_led},
	{"set_speed", cmd_set_speed},
	{"batt", cmd_get_battery},
	{"audio_play", cmd_audio_play},
	{"audio_stop", cmd_audio_stop},
	{"volume", cmd_volume},
    {NULL, NULL}
};

static THD_FUNCTION(shell_spawn_thd, p)
{
    (void) p;
    thread_t *shelltp = NULL;

    static const ShellConfig shell_cfg = {
        (BaseSequentialStream *)&SDU1,
        shell_commands
    };

    shellInit();

    while (TRUE) {
        if (!shelltp) {
            if (SDU1.config->usbp->state == USB_ACTIVE) {
                shelltp = shellCreate(&shell_cfg, SHELL_WA_SIZE, NORMALPRIO);
            }
        } else {
            if (chThdTerminatedX(shelltp)) {
                chThdRelease(shelltp);
                shelltp = NULL;
            }
        }
        chThdSleepMilliseconds(500);
    }
}


void shell_start(void)
{
    static THD_WORKING_AREA(wa, 2048);

    chThdCreateStatic(wa, sizeof(wa), NORMALPRIO, shell_spawn_thd, NULL);
}
