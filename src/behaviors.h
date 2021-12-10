#ifndef BEHAVIORS_H
#define BEHAVIORS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief   Enable obstacle avoidance.
*/
void enable_obstacle_avoidance(void);

/**
* @brief   Disable obstacle avoidance.
*/
void disable_obstacle_avoidance(void);

/**
* @brief   Starts the behaviors handling thread.
*/
void behaviors_start(void);

/**
 * @brief 	Tell whether the OA is enabled or not.
 *
 * @return	0 if OA disabled, 1 if OA enabled.
 */
uint8_t obstacle_avoidance_enabled(void);

/**
* @brief   Set desired speed for left motor.
*
* @param speed		desired forward speed in step/s when no obstacles detected
*/
void obstacle_avoidance_set_speed_left(int speed);

/**
* @brief   Set desired speed for right motor.
*
* @param speed		desired forward speed in step/s when no obstacles detected
*/
void obstacle_avoidance_set_speed_right(int speed);

#ifdef __cplusplus
}
#endif

#endif
