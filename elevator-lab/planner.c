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

#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

#include "global.h"
#include "planner.h"
#include "assert.h"
#include "semphr.h"

typedef s8 FloorEvent_t;

typedef struct {
	FloorEvent_t floor[3];	 //floor where car must go

  xSemaphoreHandle lock;          // Mutex semaphore protecting the struct

} floorEventQueue_t;

//structure holding floor request events
floorEventQueue_t floorQueue;

//checks if it is possible to stop at the given floor on the way
static bool floorInTheWay(FloorEvent_t floor);

static void plannerTask(void *params) {

  // ...

  vTaskDelay(portMAX_DELAY);

}

void setupPlanner(unsigned portBASE_TYPE uxPriority) {
	u8 i;

	//init floorQueue
	for (i=0;i<3;i++) {
		floorQueue.floor[i] = 0;
	}
	floorQueue.lock = xSemaphoreCreateMutex();

  xTaskCreate(plannerTask, "planner", 100, NULL, uxPriority, NULL);
}


//insert a floor request event into the queue
void pushFloorEvent(FloorEvent_t floor) {
	u8 i;
	
	xSemaphoreTake(floorQueue.lock, portMAX_DELAY);	
	
	//if the event is already in the queue we don't need to push it again
	for (i=0;i<3;i++) {
		if (floorQueue.floor[i] == floor) {
			xSemaphoreGive(floorQueue.lock);
			return;
		}
	}	
	
	//if the requested floor is the middle one	
	if (floor == 2) {
		//if it is possible to stop on the way
		if (floorInTheWay(floor)) {
			//stop to the floor - insert floor at the beginning of 	queue
			floorQueue.floor[2] = floorQueue.floor[1];
			floorQueue.floor[1] = floorQueue.floor[0];
			floorQueue.floor[0] = floor;
		}
	} else {
		//insert floor at the end of the queue
		//queue doesn't need to be sorted since we only ave 3 floors 
		for (i=0;i<3;i++) {
			if (floorQueue.floor[i] == 0) {
				floorQueue.floor[i] = floor;
				break;	
			}	
		}
	}

	xSemaphoreGive(floorQueue.lock);
}

bool floorInTheWay(FloorEvent_t floor) {
	

	return FALSE;
}


