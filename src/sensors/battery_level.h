#ifndef BATTERY_LEVEL_H
#define BATTERY_LEVEL_H

/** Message reprensenting a measurement of the battery level. */
typedef struct {
    float voltage;
    uint16_t raw_value;
} battery_msg_t;

/** Starts the battery measurement service. */
void battery_level_start(void);
uint16_t get_battery_raw(void);

#endif
