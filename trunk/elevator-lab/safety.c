/**
 * Program skeleton for the course "Programming embedded systems"
 *
 * Lab 1: the elevator control system
 */

/**
 * This file defines the safety module, which observes the running
 * elevator system and is able to stop the elevator in critical
 * situations
 */

#include "FreeRTOS.h"
#include "task.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_gpio.h"
#include <stdio.h>

#include "global.h"
#include "assert.h"

#define POLL_TIME (10 / portTICK_RATE_MS)

#define MOTOR_UPWARD   (TIM3->CCR1)
#define MOTOR_DOWNWARD (TIM3->CCR2)
#define MOTOR_STOPPED  (!MOTOR_UPWARD && !MOTOR_DOWNWARD)

#define STOP_PRESSED  GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_3)
#define AT_FLOOR      GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_7)
#define DOORS_CLOSED  GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_8)

#define ABS(x)        ((x) >= 0 ? (x) : (-x))

static portTickType xLastWakeTime;

static void check(u8 assertion, char *name) {
  if (!assertion) {
    printf("SAFETY REQUIREMENT %s VIOLATED: STOPPING ELEVATOR\n", name);
    for (;;) {
	  setCarMotorStopped(1);
  	  vTaskDelayUntil(&xLastWakeTime, POLL_TIME);
    }
  }
}

static void safetyTask(void *params) {
  s16 timeSinceStopPressed = -1;
  u16 timeSpeedMeasure = 0;
  s32 currentPosition = 0;
  s32 oldPosition = 0;

  xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    // Environment assumption 1: the doors can only be opened if
	//                           the elevator is at a floor and
    //                           the motor is not active

	check((AT_FLOOR && MOTOR_STOPPED) || DOORS_CLOSED,
	      "env1");

	// Environment assumption 2: The elevator moves at a maximum speed of 50cm/s
  //make the measurement every 60 ms ( max 3cm/60ms - keep the sampling point in the same place of the pulse)
  timeSpeedMeasure++;
  currentPosition = getCarPosition();
  if(timeSpeedMeasure == 6)
  { 
    check( ABS(currentPosition - oldPosition) <= 3, "env2");
    oldPosition = currentPosition;
    timeSpeedMeasure = 0;
  }
  

	// fill in environment assumption 3
  // If the ground floor is put at 0cm in an absolute coordinate system, 
  // the second floor is at 400cm and 
  // the third floor at 800cm (the at-floor sensor reports a floor with a threshold of +-0.5cm)
  if(AT_FLOOR)
  check (( currentPosition <= ( TRACKER_FLOOR1_POS + 1 )) ||  \
        (( currentPosition >= ( TRACKER_FLOOR2_POS - 1 )) && ( currentPosition <= ( TRACKER_FLOOR2_POS + 1 ))) || \
         ( currentPosition >= ( TRACKER_FLOOR3_POS - 1 )), "env3");

	// fill in your own environment assumption 4
	check(1, "env4");

    // System requirement 1: if the stop button is pressed, the motor is
	//                       stopped within 1s

	if (STOP_PRESSED) {
	  if (timeSinceStopPressed < 0)
	    timeSinceStopPressed = 0;
      else
	    timeSinceStopPressed += POLL_TIME;

      check(timeSinceStopPressed * portTICK_RATE_MS <= 1000 || MOTOR_STOPPED,
	        "req1");
	} else {
	  timeSinceStopPressed = -1;
	}

    // System requirement 2: the motor signals for upwards and downwards
	//                       movement are not active at the same time

    check(!MOTOR_UPWARD || !MOTOR_DOWNWARD,
          "req2");

	// fill in safety requirement 3
	check(1, "req3");

	// fill in safety requirement 4
	check(1, "req4");

	// fill in safety requirement 5
	check(1, "req5");

	// fill in safety requirement 6
	check(1, "req6");

	// fill in safety requirement 7
	check(1, "req7");

	vTaskDelayUntil(&xLastWakeTime, POLL_TIME);
  }

}

void setupSafety(unsigned portBASE_TYPE uxPriority) {
  xTaskCreate(safetyTask, "safety", 100, NULL, uxPriority, NULL);
}
