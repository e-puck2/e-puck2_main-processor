#ifndef IRCOMRECEIVE_H
#define IRCOMRECEIVE_H

#ifdef __cplusplus
extern "C" {
#endif

/****************************PUBLIC FUNCTIONS*************************************/
/**
* @brief	Enable reception for local communication via IR.
*/
void ircomListen(void);

/**
* @brief	Tell when a message reception is completed. Do not distinguish between discarded (failed) messages and correct messages.
*
* @return	0 if the message is not received, 1 otherwise.
*/
int ircomReceiveDone(void);

/**
* @brief	Tell if a message reception is in progress.
*
* @return	0 if no message reception is in progress, 1 otherwise.
*/
int ircomIsReceiving(void);

/**
* @brief	Tell if the robot is listening for a message or not.
*
* @return	1 if listening for a message, 0 otherwise.
*/
int ircomIsListening(void);

/**
* @brief	Enable the proximity sampling and put the local communication in an idle state.
*/
void ircomStopListening(void);

/***************************INTERNAL FUNCTIONS************************************/
/**
* @brief	FSM Receive: entry point. This should be called after a full window of samples has been acquired.
*/
void ircomReceiveMain(void);

/**
* @brief	Process the window of samples looking for an incoming message.
*/
void ircomReceiveListen(void);

/**
* @brief	FSM Receive, state 2. Receive a word.
*/
void ircomReceiveWord(void);

/**
* @brief	Find the IR sensor with most interesting signal.
*
* @return	Id of the IR sensor (0-7): refer to https://www.gctronic.com/doc/index.php?title=e-puck2#Overview for the position of the proximity sensors and their ids.
*
*/
int8_t ircomReceiveGetMaxSensor(void);

/**
* @brief   Demodulate the signal.
*
* @param rawOutput	0 if we want the number of switch counts, 1 if we want either "0" or "1" depending on the demodulated signal.
*
* @return	Either the switch counts or the interpreted bit (depending on the given parameter value).
*/
int16_t ircomReceiveDemodulate(uint8_t rawOutput);

/**
* @brief	Compute the checksum.
*
* @return	The last 2 bits of the checksum.
*
*/
int16_t ircomReceiveCheckCRC(void);

#ifdef __cplusplus
}
#endif

#endif /* IRCOMRECEIVE_H */
