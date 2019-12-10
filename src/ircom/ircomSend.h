
/*
    Copyright 2007 Alexandre Campo, Alvaro Guttierez, Valentin Longchamp.

    This file is part of libIrcom.

    libIrcom is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License.

    libIrcom is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libIrcom.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef IRCOM_SEND_H
#define IRCOM_SEND_H

/****************************PUBLIC FUNCTIONS*************************************/
/**
* @brief Start a transmission of 1 byte via IR local communication.
*
* @param value value to be sent.
*/
void ircomSend(long int value);

/**
* @brief	Tell when a message transmission is completed.
*
* @return	0 if the message is still not transmitted, 1 otherwise.
*/
int ircomSendDone(void);

/**
* @brief	Tell if a message transmission is in progress.
*
* @return	0 if no message transmission is in progress, 1 otherwise.
*/
int ircomIsSending(void);

/***************************INTERNAL FUNCTIONS************************************/
/**
* @brief	Implement pre-calculated timings for sender.
*
* @param	value value to be sent.
*/
void ircomSendCalcSending(long int value);

/**
* @brief	FSM Write: entry point.
*/
void ircomSendMain(void);

/**
* @brief	Handles the pulses based on the message to be transmitted.
*/
void ircomSendNextBit(void);

// IRCOM_SEND_H
#endif
