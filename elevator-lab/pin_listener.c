/**
 * Program skeleton for the course "Programming embedded systems"
 *
 * Lab 1: the elevator control system
 */

/**
 * Functions listening for changes of specified pins
 */

#include "FreeRTOS.h"
#include "task.h"
#include "stm32f10x_gpio.h"

#include "pin_listener.h"
#include "assert.h"


static void pollPin(PinListener *listener,
                    xQueueHandle pinEventQueue) {

  static u8 debounce_cnt[6];

  u8 data, index;


  //read GPIO Pin every 10 ms
  data = GPIO_ReadInputDataBit( listener->gpio, listener->pin );

	//map GPIO pins to indexes for the arrays
	switch (listener->pin) {
		case GPIO_Pin_0:
			index = 0;
			break;
		case GPIO_Pin_1:
			index = 1;
			break;
		case GPIO_Pin_2:
			index = 2;
			break;
		case GPIO_Pin_3:
			index = 3;
			break;
		case GPIO_Pin_7:
			index = 4;
			break;
		case GPIO_Pin_8:
			index = 5;
			break;
		default:
			break;
	} 
  
  switch (listener->status) {
    case 0: // released
      if( data == 1 ) {

        if( debounce_cnt[index] < 2 ) 
        {
          debounce_cnt[index]++;
        }
        else
        {
          listener->status = 1;  //go to pressed state
          debounce_cnt[index] = 0; //reset counter
          xQueueSend(pinEventQueue, &listener->risingEvent, portMAX_DELAY);
        }
			}
      else
      {
				
        if (debounce_cnt[index] > 2) {
					//Violation of safety env 4
					listener->status = 2;
				}	
				debounce_cnt[index] = 0;
      }
      break;

    case 1: // pressed
      if ( data == 0 )
      {

        if( debounce_cnt[index] < 2 ) 
        {
          debounce_cnt[index]++;
        }
        else
        {
					
          listener->status = 0;  //go to pressed state
          debounce_cnt[index] = 0; //reset counter
          if( listener->pin > 2 ) // ignore falling edge event for the inputs from buttons
            xQueueSend(pinEventQueue, &listener->fallingEvent, portMAX_DELAY);
        }
      }
      else
      {
				
        if (debounce_cnt[index] > 2) {
					//Violation of safety env 4
					listener->status = 2;
				}
				debounce_cnt[index] = 0;
      }
      break;
		//input behaved incorrectly - will be noticed by the Safety module
		case 2:
			//do nothing
			break;
    default:
      break;

  }

}

static void pollPinsTask(void *params) {
  PinListenerSet listeners = *((PinListenerSet*)params);
  portTickType xLastWakeTime;
  int i;

  xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    for (i = 0; i < listeners.num; ++i)
	  pollPin(listeners.listeners + i, listeners.pinEventQueue);
    
	vTaskDelayUntil(&xLastWakeTime, listeners.pollingPeriod);
  }
}

void setupPinListeners(PinListenerSet *listenerSet) {
  portBASE_TYPE res;

  res = xTaskCreate(pollPinsTask, "pin polling",
                    100, (void*)listenerSet,
					listenerSet->uxPriority, NULL);
  assert(res == pdTRUE);
}
