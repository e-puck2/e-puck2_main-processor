
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

#ifndef IRCOM_TOOLS_H
#define IRCOM_TOOLS_H

/**
* @brief Give an estimate of the distance from the emitting robot (new version with an updated coefficients table).
* @param irPerceived sensor id: refer to https://www.gctronic.com/doc/index.php?title=e-puck2#Overview for the position of the proximity sensors and their ids.
* @return ditance in cm.
*/
float ircomEstimateDistanceNew(int irPerceived);

/**
* @brief Give an estimate of the distance from the emitting robot.
* @param irPerceived sensor id: refer to https://www.gctronic.com/doc/index.php?title=e-puck2#Overview for the position of the proximity sensors and their ids.
* @return ditance in cm.
*/
float ircomEstimateDistance (int irPerceived);

/**
* @brief Give the angle of the sensor passed as argument. This is used to get the angle of the sensor that is receiving the message.
* @param middleSensor sensor id: refer to https://www.gctronic.com/doc/index.php?title=e-puck2#Overview for the position of the proximity sensors and their ids.
* @return angle in radians.
*/
float ircomEstimateDirection (int middleSensor);

/**
* @brief Integer to binary conversion.
* @param n input value as integer
* @param w output value binary representation.
*/
void ircomInt2Bin (long int n, ircomWord w);

/**
* @brief Binary to integer conversion.
* @param w input value as binary representation
* @return output value as integer.
*/
long int ircomBin2Int(volatile ircomWord w);

/**
* @brief 	Get the current time ticks that are increased in the timer14 interrupt.
*			When in reception mode the interrupt has an interval of 100 us.
*			When in transmission mode it as an interval of either 260 us or 520 us depending on the bit that need to be transferred.
* @return	timer ticks
*/
long int ircomGetTime(void);

/**
* @brief Reset the time ticks.
*/
void ircomResetTime (void);

// IRCOM_TOOLS_H
#endif
