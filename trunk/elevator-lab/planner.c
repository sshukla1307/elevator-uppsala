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

typedef struct {
	FloorEvent_t floor[3];	 //floor where car must go

  xSemaphoreHandle lock;          // Mutex semaphore protecting the struct

} floorEventQueue_t;

extern xQueueHandle pinEventQueue;
//structure holding floor request events
floorEventQueue_t floorQueue;

FloorEvent_t targetfloor = FLOOR1;

//pull a floor request event from the queue
static FloorEvent_t popFloorEvent(void);
//insert a floor request event into the queue
static void pushFloorEvent(FloorEvent_t floor);
//checks if it is possible to stop at floor 2 on the way to floor 1 or 3
static bool floor2InTheWay(void);

static void plannerTask(void *params) {

	PinEvent ev;
	FloorEvent_t currentfloor = FLOOR1, tempTargetFloor;
  bool floorreached = TRUE, doors_closed = FALSE;
  Direction dir = Unknown;
  static u32 timeout = 0;
  static u8 tmp = 0;
  
	portTickType xLastWakeTime;
	// Initialise the xLastWakeTime variable with the current time.
	xLastWakeTime = xTaskGetTickCount();

 


	for(;;) {    

    // check event queue for new event
		while (xQueueReceive(pinEventQueue, &ev, (portTickType)0 ) == pdTRUE) {
			switch(ev) {

				case TO_FLOOR_1:
          // check current floor
          if(targetfloor != FLOOR1) 
					  pushFloorEvent(FLOOR1);    // set the floor request only if it isn't the current floor
					break;	

				case TO_FLOOR_2:
          if(targetfloor != FLOOR2)
  					pushFloorEvent(FLOOR2);    // set the floor request only if it isn't the current floor
					break;

				case TO_FLOOR_3:
          if(targetfloor != FLOOR3)
					  pushFloorEvent(FLOOR3);    // set the floor request only if it isn't the current floor
          break;					 
				
				case ARRIVED_AT_FLOOR:
            floorreached = TRUE;
					break;
					
				case LEFT_FLOOR:
            floorreached = FALSE;
					break;

				case DOORS_CLOSED:
            doors_closed = TRUE;
					break;

				case DOORS_OPENING:
            doors_closed = FALSE;
					break;

				case STOP_PRESSED:
            setCarMotorStopped(1);
					break;

				case STOP_RELEASED:
            //setCarMotorStopped(0);
					break;

				default:
					break;

			}
		}

    /* only set the target when doors are closed  */
    

    dir = getCarDirection();                        
    if(floorreached && ( dir == Unknown ) && (timeout < FLOOR_TIMEOUT))
    {
      timeout++;  // increment timer if floor is reached and doors are not oppened
      tmp = 0;

      // test if lift reached the target floor and pop the request from the queue
      if( targetfloor != currentfloor ){
         currentfloor = popFloorEvent();
      }

    }
    else
    {

      if( doors_closed )
      {
        // read new target floor
				tempTargetFloor = readFloorEvent(); 
				if (tempTargetFloor != UNKNOWN) {
        	targetfloor = tempTargetFloor;
				}
      
        // set target position accordingly
        switch(targetfloor)
        {
          case FLOOR1:
            setCarTargetPosition(TRACKER_FLOOR1_POS);
            break;
          case FLOOR2:
            setCarTargetPosition(TRACKER_FLOOR2_POS);
            break;                                   
          case FLOOR3:
            setCarTargetPosition(TRACKER_FLOOR3_POS);
            break;
    
          default:
            break;
        }
      }

      // wait for the motor task to change the direction
      if(tmp > 50 )
        timeout = 0;
      else
        tmp++;
        
    
//      // test if lift reached the target floor and pop the request from the queue
//      if(( floorreached && ( dir == Unknown ) && ( !doors_closed || (timeout > FLOOR_TIMEOUT))) && ( targetfloor != currentfloor ))
//      //if(( floorreached && ((timeout > FLOOR_TIMEOUT))) && ( targetfloor != currentfloor ))
//      {
//         // set the new current floor and pop out the event from queue        
//         currentfloor = getFloorEvent();
//  
//         timeout = 0;
//      }
    
    }


    

   

		vTaskDelayUntil(&xLastWakeTime, 10 / portTICK_RATE_MS);
	}

}

void setupPlanner(unsigned portBASE_TYPE uxPriority) {
	u8 i;

	floorQueue.lock = xSemaphoreCreateMutex();
	//init floorQueue
	for (i=0;i<3;i++) {
		floorQueue.floor[i] = UNKNOWN;
	}

  xTaskCreate(plannerTask, "planner", 100, NULL, uxPriority, NULL);
}

FloorEvent_t readFloorEvent(void) {
	FloorEvent_t floor;
		
	xSemaphoreTake(floorQueue.lock, portMAX_DELAY);
 	floor = floorQueue.floor[0];
	xSemaphoreGive(floorQueue.lock);

  if( floor != UNKNOWN ) {
		//return the first element in the queue
    return floorQueue.floor[0];
  }
  else
  {
		//else return current floor
    return targetfloor;
  }
}

//pull a floor request event from the queue
FloorEvent_t popFloorEvent(void) {
	FloorEvent_t floor;

	xSemaphoreTake(floorQueue.lock, portMAX_DELAY);

	//get the first element in the queue
	floor = floorQueue.floor[0];
	//shift queue to the left by 1 position
	floorQueue.floor[0] = floorQueue.floor[1];
	floorQueue.floor[1] = floorQueue.floor[2];
  floorQueue.floor[2] = UNKNOWN;

	xSemaphoreGive(floorQueue.lock);

	return floor;
}

//insert a floor request event into the queue
void pushFloorEvent(FloorEvent_t floor) {
	u8 i;
  Direction dir = getCarDirection();
	
	xSemaphoreTake(floorQueue.lock, portMAX_DELAY);

	//if the event is already in the queue we don't need to push it again
	for (i=0;i<3;i++) {
		if (floorQueue.floor[i] == floor) {
			xSemaphoreGive(floorQueue.lock);
			return;
		}
	}	
	
	//if the requested floor is the middle one and lift is mooving
	if (floor == FLOOR2)  {
		//if it is possible to stop on the way
		if (floor2InTheWay()) {
			//stop to the floor - insert floor at the beginning of 	queue
			floorQueue.floor[2] = floorQueue.floor[1];
			floorQueue.floor[1] = floorQueue.floor[0];
			floorQueue.floor[0] = floor;
			xSemaphoreGive(floorQueue.lock);
			return;
		}
	} 
	
	//insert floor at the end of the queue
	//queue doesn't need to be sorted since we only have 3 floors 
	for (i=0;i<3;i++) {
		if (floorQueue.floor[i] == UNKNOWN) {
			floorQueue.floor[i] = floor;
			break;	
		}	
	}

	xSemaphoreGive(floorQueue.lock);
}

bool floor2InTheWay() {

	s32 position = getCarPosition();
	Direction dir = getCarDirection();
	
	//if going up and can stop safely
	if (((dir == Up) && (position + SAFE_STOP_DISTANCE <= TRACKER_FLOOR2_POS)) ||
	//if going down and can stop safely
		 ((dir == Down) && (position - SAFE_STOP_DISTANCE >= TRACKER_FLOOR2_POS)))	{
		return TRUE;
	}
	return FALSE;
}


