/**
 * Program skeleton for the course "Programming embedded systems"
 *
 * Lab 1: the elevator control system
 */

/**
 * The planner module, which is responsible for consuming
 * pin/key events, and for deciding where the elevator
 * should go next
 */

#ifndef PLANNER_H
#define PLANNER_H

typedef u8 floorEventQueue_t[3];

extern floorEventQueue_t floorQueue;


void setupPlanner(unsigned portBASE_TYPE uxPriority);

#endif
