#ifndef IRCOM_H
#define IRCOM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define PROXIMITY_NB_CHANNELS 8

#define IRCOM_FSM_IDLE 0
#define IRCOM_FSM_RECEIVE 1
#define IRCOM_FSM_SEND 2

#define IRCOM_WORDSIZE 8 // bits
#define IRCOM_SAMPLING_WINDOW 32
#define IRCOM_DETECTION_THRESHOLD_AMPLITUDE 75 //50 // min amplitude of signal -> this is limiting the communication range
#define IRCOM_DETECTION_THRESHOLD_SWITCH 2 //4 // min number of switch to initiate message
#define IRCOM_MARK 0
#define IRCOM_SPACE 1
#define IRCOM_MARK_THRESHOLD 8 // min number of signal switch
#define IRCOM_SPACE_THRESHOLD 4 // min number of signal switch

typedef char ircomWord[IRCOM_WORDSIZE + 4];

// message queue
typedef struct
{
    long int value;
    float distance;
    float direction;
    int receivingSensor;
    int error; // -1 = inexistent msg, 0 = all is ok, 1 = error in transmission
} IrcomMessage;

// structure that keeps all relevant information to run ircom module
// mainly which FSM is running
#define IRCOM_FSM_IDLE 0
#define IRCOM_FSM_RECEIVE 1
#define IRCOM_FSM_SEND 2
#define IRCOM_MESSAGES_MAX 20
typedef struct
{
    int paused;
    int fsm;

    IrcomMessage messages[IRCOM_MESSAGES_MAX];
    int messagesBegin;
    int messagesEnd;
    int messagesCount;

    long int time;
} Ircom;

// structure that keeps track of all information related to receiveing
typedef struct
{
    // public :
    int receiving;
    int done;
    int error;
    ircomWord word;
    float distance;
    float direction;

    // private :
    int state;
    int currentBit;
    int receivingSensor;

    int continuousListening;
} IrcomReceive;

// structure that keeps track of all information related to writing
#define IRCOM_SEND_OMNI 0
#define IRCOM_SEND_DIRECTIONAL 1
typedef struct
{
    // public :
    int done;
    ircomWord word;
    int type; // omni or directional
    int sensor;

    // private :
    int pulseState;
    int switchCounter;
    int switchCountMax;
    int signal;
    int currentBit;

    int markSwitchCount;
    int spaceSwitchCount;
    double markDuration;
    double spaceDuration;

    double durations[IRCOM_WORDSIZE+4];
    int switchCounts[IRCOM_WORDSIZE+4];
    int interruptedListening;
} IrcomSend;

extern int16_t *window_data;
extern volatile unsigned int ircom_last_ir_scan_id;
extern volatile Ircom ircomData;
extern volatile IrcomReceive ircomReceiveData;
extern volatile IrcomSend ircomSendData;
extern volatile uint8_t prox_sampling_enabled;

 /**
 * @brief   Configure and start the peripherals needed for the IR local communication.
 * 			This function need to be called once at the beginning and we need to be sure that the "proximity" module is stopped (with the "proximity_stop" function)
 * 			before calling this function otherwise there will be conflicts.
 * 			This function also start a thread that broadcasts the measures through a proximity_msg_t message on the /proximity topic
 */
void ircomStart(void);

/**
* @brief   Stop the timer that operates the local communication. No IR local communication functions are anymore available.
*/
void ircomStop(void);

/**
* @brief   Restart the timer that operates the local communication. The IR local communication functions are now available.
*/
void ircomRestart(void);

/**
* @brief   Put in standby the IR local communication.
* @param status 0 = normal, 1 = pause
*/
void ircomPause(int status);

/**
* @brief   Enable proximity sampling together with the IR local communication. The sensors will be sampled at most at 5 Hz.
*/
void ircomEnableProximity(void);

/**
* @brief   Disable proximity sampling, only the IR local communication is available.
*/
void ircomDisableProximity(void);

/**
* @brief   The robot will continuously listen for new messages.
*/
void ircomEnableContinuousListening(void);

/**
* @brief   The robot will stop after receiving one message.
*/
void ircomDisableContinuousListening(void);

/**
* @brief   The robot will send the message through all the sensors.
*/
void ircomSetOmnidirectional(void);

/**
* @brief   The robot will send the message through one sensor only.
*
* @param sensor Sensors id from which the message will be sent.
*/
void ircomSetDirectional(int sensor);

/**
* @brief   Reset the sampling window data.
*/
void ircom_scan_reset(void);

/**
* @brief   Tell whether a sampling window is available or not. It clears also the "filled" flag.
*
* @return 1 if the sampling window is available, 0 otherwise
*/
uint8_t ircom_is_array_filled(void);

/**
* @brief   Skip an amount of samples, this samples will not be inserted in the sampling window. Used for communication synchronization.
*
* @param samples_count Number of samples to skip.
*/
void ircom_skip_samples(int samples_count);

/**
* @brief   Turn the ADC off. No sampling is performed.
*/
void ircom_adc_off(void);

/**
* @brief   Turn the ADC on.
*/
void ircom_adc_on(void);

/**
* @brief	Runs the IR sensor calibration process
*/
void ircom_calibrate_ir(void);

/**
* @brief	Returns the last value measured by the chosen sensor
*
* @param	sensor_number 0-7
*
* @return	Last value measured by the sensor
*/
int ircom_get_prox(unsigned int sensor_number);

/**
* @brief	Returns the calibration value for the chosen sensor
*
* @param	sensor_number 0-7
*
* @return	Calibration value of the sensor
*/
int ircom_get_calibrated_prox(unsigned int sensor_number);

/**
* @brief	Returns the last ambiant light value measured by the chosen sensor
*
* @param	sensor_number 0-7
*
* @return	Last ambiant light value measured by the sensor
*/
int ircom_get_ambient_light(unsigned int sensor_number);

#ifdef __cplusplus
}
#endif

#endif /* IRCOM_H */
