#include <stdio.h>
#include <string.h>
#include "ch.h"
#include "hal.h"
#include "ircomReceive.h"
#include "ircom.h"
#include "ircomTools.h"
#include "ircomMessages.h"


/***************************INTERNAL FUNCTIONS************************************/
void ircomReceiveMain(void) {

    switch (ircomReceiveData.state) {
    	case 0 :
    		ircomReceiveListen();
    		break;
    	case 1 :
    		ircomReceiveWord();
    		break;
    }
}

void ircomReceiveListen(void) {
    // process signal only when a full window is available
    if (ircom_is_array_filled() == 0) {
    	return;
    }

    int maxSensor = ircomReceiveGetMaxSensor();

    // no significant modulation perceived ?
    if (maxSensor < 0) {
    	return;
    }

    int switchCount = -1;
    ircomReceiveData.receivingSensor = maxSensor;
    if (maxSensor >= 0)
		switchCount = ircomReceiveDemodulate(1);	// return switch count

    // no significant signal perceived ?
    if (switchCount < IRCOM_DETECTION_THRESHOLD_SWITCH)	// 2
    {
		return;
    }

    // first thing, find out the shift of the signal
    int shift, u;
    int min = 4096;
    int max = 0;
    for (shift = 0, u = maxSensor; shift < IRCOM_SAMPLING_WINDOW; shift++, u += PROXIMITY_NB_CHANNELS)
    {
		int s = window_data[u];
		if (s > max)
		    max = s;
		if (s < min)
		    min = s;
		if (max - min >= IRCOM_DETECTION_THRESHOLD_AMPLITUDE)
		    break;
    }
    // skip samples, restart sampling the 3rd bit only (2nd is already on the way, not synced)
    ircom_skip_samples(IRCOM_SAMPLING_WINDOW - ircom_last_ir_scan_id + shift);	// ircom_last_ir_scan_id corresponds to the index of the current sample in the window sampling array
																				// Basically we get the 1st bit, we process it detecting a start bit and the related shift, but
																				// in the meanwhile the 2nd bit is being sampled, thus:
																				// - finish sampling the current window related to the the 2nd bit => IRCOM_SAMPLING_WINDOW - ircom_last_ir_scan_id
																				// - skip the samples to be synced => + shift

    // prevent ad from triggering prox sampling
    ircomDisableProximity();

    // ok setup everything : start receiveing word, record all
    ircomReceiveData.currentBit = 0;
    ircomReceiveData.receivingSensor = maxSensor;
    ircomReceiveData.done = 0;
    ircomReceiveData.error = 0;
    ircomReceiveData.receiving = 1;
    ircomReceiveData.state = 1;
}

// FSM Receive, state 2
// receive a word
void ircomReceiveWord(void) {
    // process signal only when a full window is available
    if (ircom_is_array_filled() == 0) {
    	return;
    }

    int signal = ircomReceiveDemodulate(0);	// IRCOM_MARK=0 or IRCOM_SPACE=1

    // no significant signal perceived ?
    if (signal < 0)
    {
		// stop receiveing, discard message
		ircomReceiveData.done = 1;
		ircomReceiveData.error = 1;
		ircomReceiveData.receiving = 0;
		ircomReceiveData.state = 0;

		// reenable prox in ad
		ircomEnableProximity();

		// set fsm in idle mode (except continuous listening mode...)
		if (ircomReceiveData.continuousListening == 1)
		    ircomListen();
		else
		    ircomData.fsm = IRCOM_FSM_IDLE;

		return;
    }

    ircomReceiveData.word[ircomReceiveData.currentBit++] = signal;

    // end of message ?
    if (ircomReceiveData.currentBit >= IRCOM_WORDSIZE + 2) // 8 bit data + 2 bit crc
    {
		// evaluate CRC
		ircomReceiveData.error = ircomReceiveCheckCRC();

		// stop receiving
		ircomReceiveData.done = 1;
		ircomReceiveData.receiving = 0;
		ircomReceiveData.state = 0;
		//Set distance and direction
		ircomReceiveData.distance = ircomEstimateDistanceNew((int)ircomReceiveData.distance);
		ircomReceiveData.direction = ircomEstimateDirection(ircomReceiveData.receivingSensor);

		// record in the msg queue
		long int value = ircomBin2Int(ircomReceiveData.word);
		ircomPushMessage(value,
				 ircomReceiveData.distance,
				 ircomReceiveData.direction,
				 ircomReceiveData.receivingSensor,
				 ircomReceiveData.error);


		// reenable prox in ad
		ircomEnableProximity();

		// set fsm in idle mode (except continuous listening mode...)
		if (ircomReceiveData.continuousListening == 1)
		    ircomListen();
		else
		    ircomData.fsm = IRCOM_FSM_IDLE;

		return;
    }
}

int8_t ircomReceiveGetMaxSensor(void) {
    //find the ir sensor with most interesting signal
    int i;

    int maxDiff = 0;
    int maxSensor = -1;

    for (i = 0; i < PROXIMITY_NB_CHANNELS; i++) {
		int index = i, j = 0, min = 4096, max = 0;
		for (j = 0; j < IRCOM_SAMPLING_WINDOW; j++)
		{
		    int v = window_data[index];
		    if (v < min)
		    {
				min = v;
		    }
		    else if (v > max)
		    {
				max = v;
		    }
		    index += PROXIMITY_NB_CHANNELS;
		}

		if (max - min > maxDiff)
		{
		    maxDiff = max - min;
		    maxSensor = i;
		}
    }

    if (maxDiff < IRCOM_DETECTION_THRESHOLD_AMPLITUDE)
	return -1;

    return maxSensor;
}

int16_t ircomReceiveDemodulate(uint8_t rawOutput) {
    int maxSensor = ircomReceiveData.receivingSensor;

    // demodulate signal
    //now we have the threshold and can normalize the signal if needed
    static int rs[IRCOM_SAMPLING_WINDOW];
    int i, u;
    for (i = 0, u = maxSensor; i < IRCOM_SAMPLING_WINDOW; i++, u += PROXIMITY_NB_CHANNELS)
    {
		rs[i] = window_data[u];
    }

    // find max amplitude of signal
    int min = 4096, max = 0;
    for (i = 0; i < IRCOM_SAMPLING_WINDOW; i++)
    {
		if (rs[i] < min)
		{
		    min = rs[i];
		}
		else if (rs[i] > max)
		{
		    max = rs[i];
		}
    }
    if (max - min < IRCOM_DETECTION_THRESHOLD_AMPLITUDE)
	return -1;

    // compute mean signal
    long int tmp = 0;
    for (i = 0; i < IRCOM_SAMPLING_WINDOW; i++)
    {
		tmp += rs[i];
    }
    int mean = (int)(tmp / IRCOM_SAMPLING_WINDOW);

    // substract mean from signal
    for (i = 0; i < IRCOM_SAMPLING_WINDOW; i++)
    {
		rs[i] -= mean;
    }

    // start counting number of switch around mean signal
    int signalState;
    if (rs[0] > 0)
    	signalState = 1;
    else
    	signalState = -1;

    int switchCount = 0;

    for (i = 1; i < IRCOM_SAMPLING_WINDOW; i++)
    {
		if(rs[i] > 0)
		{
		    if (signalState < 0)
		    {
				signalState = 1;
				switchCount++;
		    }
		}
		else
		{
		    if (signalState > 0)
		    {
				signalState = -1;
				switchCount++;
		    }
		}
    }

    if (rawOutput)
    	return switchCount;

    if (switchCount >= IRCOM_MARK_THRESHOLD)	//8
	{
		ircomReceiveData.distance = max - min;
		return IRCOM_MARK;
	}
    else if (switchCount >= IRCOM_SPACE_THRESHOLD) //4
		return IRCOM_SPACE;
    else
	return -1;
}

int16_t ircomReceiveCheckCRC(void) {
    // compute checksum
    int i;
    int bitSum = 0;
    for(i = 0; i < IRCOM_WORDSIZE; i++)
    {
		if (ircomReceiveData.word[i] == IRCOM_SPACE)
	   		bitSum++;
    }
    int crc = ircomReceiveData.word[i] * 2 + ircomReceiveData.word[i + 1];

    // sum and get only the last 2 bits
    return ((crc + bitSum) & 0x003);
}

/*************************END INTERNAL FUNCTIONS**********************************/



/****************************PUBLIC FUNCTIONS*************************************/
void ircomListen(void) {
    // stop processing messages
    ircomPause(1);

    // initialise FSM
    ircomReceiveData.done = 0;
    ircomReceiveData.receiving = 0;
    ircomReceiveData.state = 0;
    ircomData.fsm = IRCOM_FSM_RECEIVE;
    ircomReceiveData.distance = -1;

    // set interrupt trigger speed
    gptChangeIntervalI(&GPTD14, 100);
	
    // process messages again
    ircomPause(0);
}

// interface for end user
int ircomReceiveDone(void) {
    return ircomReceiveData.done;
}

int ircomIsReceiving(void) {
    if (ircomReceiveData.receiving == 1) {
    	return 1;
    } else {
    	return 0;
    }
}

int ircomIsListening(void) {
    if (ircomData.fsm == IRCOM_FSM_RECEIVE)
	return 1;
    else 
	return 0;
}

void ircomStopListening(void) {
    // stop receiving
    ircomReceiveData.done = 0;
    ircomReceiveData.receiving = 0;
    ircomReceiveData.state = 0;

    // reenable prox in ad
    ircomEnableProximity();

    // set fsm in idle mode
    ircomData.fsm = IRCOM_FSM_IDLE;
}

/**************************END PUBLIC FUNCTIONS***********************************/


