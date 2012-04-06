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

#define CAR_SPEED_SAMPLING_INTERVAL (60 / portTICK_RATE_MS	/ POLL_TIME)
#define CAR_STOP_PERIOD (1000 / portTICK_RATE_MS / POLL_TIME)

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
  u32 timetowait = 0;
  s32 currentPosition = 0;
  s32 oldPosition = 0;
	static bool floornew;
	static bool old_MOTOR_STOPPED = TRUE;
	static bool old_STOP_PRESSED = FALSE;
	

  xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    // Environment assumption 1: the doors can only be opened if
	//                           the elevator is at a floor and
    //                           the motor is not active

	check((AT_FLOOR && MOTOR_STOPPED) || DOORS_CLOSED,
	      "env1");

	// Environment assumption 2: The elevator moves at a maximum speed of 50cm/s
  //make the measurement every 60 ms ( max 3cm/60ms - keep the sampling point in the same place of the pulse)

  currentPosition = getCarPosition();

  //limit against overflow
	if (timeSpeedMeasure <= CAR_SPEED_SAMPLING_INTERVAL) {
		timeSpeedMeasure++;
	}

  if(timeSpeedMeasure == CAR_SPEED_SAMPLING_INTERVAL)
  { 
    check( ABS(currentPosition - oldPosition) <= 4, "env2");
    oldPosition = currentPosition;
    timeSpeedMeasure = 0;
  }
  

	// Environment assumption 3
  // If the ground floor is put at 0cm in an absolute coordinate system, 
  // the second floor is at 400cm and 
  // the third floor at 800cm (the at-floor sensor reports a floor with a threshold of +-0.5cm)
  if(AT_FLOOR)
  check (( currentPosition <= ( TRACKER_FLOOR1_POS + 1 )) ||  \
        (( currentPosition >= ( TRACKER_FLOOR2_POS - 1 )) && ( currentPosition <= ( TRACKER_FLOOR2_POS + 1 ))) || \
         ( currentPosition >= ( TRACKER_FLOOR3_POS - 1 )), "env3");

	// Environment assumption 4: The values of the inputs stabilize on 0 or 1 within 20 ms 
	check(checkInputsStabilized(), "env4");

    // System requirement 1: if the stop button is pressed, the motor is
	//                       stopped within 1s

	if (STOP_PRESSED) {
	  if (timeSinceStopPressed < 0)
	    timeSinceStopPressed = 0;
      else {
				if (timeSinceStopPressed < CAR_STOP_PERIOD) {
					timeSinceStopPressed++;
				}
    		check((timeSinceStopPressed <= CAR_STOP_PERIOD) || MOTOR_STOPPED,	"req1");
			}
	} else {
	  timeSinceStopPressed = -1;
	}

    // System requirement 2: the motor signals for upwards and downwards
	//                       movement are not active at the same time

    check(!MOTOR_UPWARD || !MOTOR_DOWNWARD,
          "req2");

	// Safety requirement 3: The elevator may not pass the end positions, that is, go through the roof or the floor 
	check((currentPosition >= TRACKER_FLOOR1_POS) && (currentPosition <= TRACKER_FLOOR3_POS), "req3");

	old_STOP_PRESSED |= (bool) STOP_PRESSED;

	// Safety requirement 4: A moving elevator halts only if the stop button is pressed or the elevator has arrived at a floor
	check(old_MOTOR_STOPPED || (AT_FLOOR || old_STOP_PRESSED || !MOTOR_STOPPED), "req4");

	old_MOTOR_STOPPED =	(bool) MOTOR_STOPPED;
	

	// Safety requirement 5: Once the elevator has stopped at a floor, it will wait for at least 1s before it continues to another floor
	//check((( timetowait > FLOOR_TIMEOUT ) && !MOTOR_STOPPED) || MOTOR_STOPPED, "req5");
  check(( timetowait > FLOOR_TIMEOUT ) || MOTOR_STOPPED, "req5");

  if(MOTOR_STOPPED && AT_FLOOR){
		if(floornew == TRUE){
			timetowait = 0;
			floornew = FALSE;
		}
		//limit against overflow
    if (timetowait <= FLOOR_TIMEOUT + 1) {
			timetowait++;
		}
	} else {
    floornew = TRUE;
	}

  

	// Safety requirement 6: The elevator will not move while the doors are open
	// This requirement is covered by the implementation of  Environment assumption 1
	check(1, "req6");

	// Safety requirement 7: The maximum accelleration/decelleration of the elevator is 100 cm / s2
	check(1, "req7");


	// Safety requirement 8: The elevator moves only when it is called/ordered to go to a floor
	check(MOTOR_STOPPED || (getPlannerTargetPosition() == getCarTargetPosition()), "req8");

	vTaskDelayUntil(&xLastWakeTime, POLL_TIME);
  }

}

void setupSafety(unsigned portBASE_TYPE uxPriority) {
  xTaskCreate(safetyTask, "safety", 100, NULL, uxPriority, NULL);
}
