#include "utility.h"
#include "..\sensors\battery_level.h"
#include "..\selector.h"

//#define MAX_BATT_VALUE 2560 // corresponds to 4.2 volts
//#define MIN_BATT_VALUE 2070 // corresponds to 3.4 volts
//#define BATT_VALUES_RANGE (MAX_BATT_VALUE-MIN_BATT_VALUE)

void wait(long num) {
	long i;
	for(i=0;i<num;i++);
}

int getselector(void) {
	return get_selector();
}

unsigned int getBatteryValueRaw(void) {
	return get_battery_raw();
}

unsigned int getBatteryValuePercentage(void) {
//    if(isEpuckVersion1_3()) {
//        return 100*(float)(e_acc_scan[2][e_last_acc_scan_id]-MIN_BATT_VALUE)/(float)BATT_VALUES_RANGE;
//    } else {
//        return 0;
//    }
    return 0;
}

void resetTime(void) {
//    tickAdcIsr = 0;
}

// Based on ADC ISR interrupt frequency of 1/16384 (about 61 us). Each time the ISR is entered the tick
// is incremented, so we can compute the elapsed time from the last reset (knowing the ISR period)
float getDiffTimeMs(void) {
//    return ((float)tickAdcIsr)*ADC_ISR_PERIOD_MS;   // the function itself takes 20-40 us (negligable)
	return 0.0;
}

float getDiffTimeMsAndReset(void) {
//    float value = ((float)tickAdcIsr)*ADC_ISR_PERIOD_MS;   // the function itself takes 20-40 us (negligable)
//    tickAdcIsr = 0;
//    return value;
    return 0.0;
}

