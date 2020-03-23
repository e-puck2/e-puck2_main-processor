static inline mp_uint_t mp_hal_ticks_ms(void) { return 0; }
// static inline void mp_hal_set_interrupt_char(char c) {}
void mp_hal_set_interrupt_char(int c);

int mp_is_terminal_connected(void);

void mp_hal_stdin_rx_flush(void);