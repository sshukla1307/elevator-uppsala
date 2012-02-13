/**
 * Program skeleton for the course "Programming embedded systems"
 *
 * Lab 1: the elevator control system
 */

/**
 * Class for keeping track of the car position.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "global.h"
#include "position_tracker.h"

#include "assert.h"

static void positionTrackerTask(void *params) {
	portTickType xLastWakeTime;

	static bool pulseHigh = FALSE;

	PositionTracker *tracker = (PositionTracker*)params;

	// Initialise the xLastWakeTime variable with the current time.
	xLastWakeTime = xTaskGetTickCount();

	for (;;) {

		if ((GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_9)) && (pulseHigh == FALSE)) {

			//new pulse detected
			pulseHigh = TRUE;

		  xSemaphoreTake(tracker->lock, portMAX_DELAY);

			if (tracker->direction == Up)
        tracker->position++;
      else if( tracker->direction == Down )
        tracker->position--;
      else ;  //do nothing

			xSemaphoreGive(tracker->lock);
		}
    else
      pulseHigh = FALSE; //reset pulse flag, wait for new pulse

  	//delay for 3 ms since started
  	vTaskDelayUntil(&xLastWakeTime, tracker->pollingPeriod);
  }

}

void setupPositionTracker(PositionTracker *tracker,
                          GPIO_TypeDef * gpio, u16 pin,
						  portTickType pollingPeriod,
						  unsigned portBASE_TYPE uxPriority) {
  portBASE_TYPE res;

  tracker->position = 0;
  tracker->lock = xSemaphoreCreateMutex();
  assert(tracker->lock != NULL);
  tracker->direction = Unknown;
  tracker->gpio = gpio;
  tracker->pin = pin;
  tracker->pollingPeriod = pollingPeriod;

  res = xTaskCreate(positionTrackerTask, "position tracker",
                    80, (void*)tracker, uxPriority, NULL);
  assert(res == pdTRUE);
}

void setDirection(PositionTracker *tracker, Direction dir) {

  xSemaphoreTake(tracker->lock, portMAX_DELAY);

	//set the tracker direction
	tracker->direction = dir;

	xSemaphoreGive(tracker->lock);

}

Direction getDirection(PositionTracker *tracker) {

	Direction dir;

  xSemaphoreTake(tracker->lock, portMAX_DELAY);

	//get the tracker direction
	dir = tracker->direction;

	xSemaphoreGive(tracker->lock);

	return dir;

}

s32 getPosition(PositionTracker *tracker) {

	s32 aux;

  xSemaphoreTake(tracker->lock, portMAX_DELAY);

	//read the car's position
	aux = tracker->position;

	xSemaphoreGive(tracker->lock);

  return aux;

}

