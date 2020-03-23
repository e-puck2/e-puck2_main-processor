#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "lib/utils/pyexec.h"
#include "mphalport.h"
#include "lib/mp-readline/readline.h"
#include "mpport.h"



mp_lexer_t *mp_lexer_new_from_file(const char *filename) {
    mp_raise_OSError(MP_ENOENT);
}

mp_import_stat_t mp_import_stat(const char *path) {
    return MP_IMPORT_STAT_NO_EXIST;
}

mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);

void nlr_jump_fail(void *val) {
    while (1);
}

bool micropython_parse_compile_execute_from_str(const char* str){
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_obj_t module_fun;
        mp_lexer_t *lex;
        lex = mp_lexer_new_from_str_len(1, str, strlen(str), 0);
        qstr source_name = 1; /*MP_QSTR_*/
        mp_parse_tree_t parse_tree = mp_parse(lex, MP_PARSE_FILE_INPUT);
        module_fun = mp_compile(&parse_tree, source_name, 0);
        mp_hal_set_interrupt_char(CHAR_CTRL_C); // allow ctrl-C to interrupt us
        mp_call_function_0(module_fun);
        mp_hal_set_interrupt_char(-1); // disable interrupt
        mp_handle_pending(true); // handle any pending exceptions (and any callbacks)
        nlr_pop();
        return true;
    }else{
        return false;
    }
}