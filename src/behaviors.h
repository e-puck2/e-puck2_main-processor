#ifndef BEHAVIORS_H
#define BEHAVIORS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief   Enable obstacle avoidance (forward motion only).
*
* @param speed		desired forward speed in step/s when no obstacles detected
*/
void enable_obstacle_avoidance(int speed);

/**
* @brief   Disable obstacle avoidance.
*/
void disable_obstacle_avoidance(void);

/**
* @brief   Starts the behaviors handling thread.
*/
void behaviors_start(void);

#ifdef __cplusplus
}
#endif

#endif
