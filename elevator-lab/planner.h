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

typedef enum {
	UNKNOWN = 0,
	FLOOR1 = 1,
	FLOOR2 = 2,
	FLOOR3 = 3
} FloorEvent_t;

void setupPlanner(unsigned portBASE_TYPE uxPriority);

//reads an event from the queue without popping it out
FloorEvent_t readFloorEvent(void);

#endif
