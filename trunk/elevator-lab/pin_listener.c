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

#include "pin_listener.h"
#include "assert.h"


static void pollPin(PinListener *listener,
                    xQueueHandle pinEventQueue) {

  static u8 debounce_cnt[9];
  u8 data;


  //read GPIO Pin every 10 ms
  data = GPIO_ReadInputDataBit( listener->gpio, listener->pin );
  
  switch (listener->status) {
    case 0: // released
      if( data == 1 )
        if( debounce_cnt[listener->pin] < 2 ) 
        {
          debounce_cnt[listener->pin]++;
        }
        else
        {
          listener->status = 1;  //go to pressed state
          debounce_cnt[listener->pin] = 0; //reset counter
          xQueueSend(pinEventQueue, &listener->risingEvent, portMAX_DELAY);
        }
      break;

    case 1: // pressed
      if ( data == 0 )
      {
        if( debounce_cnt[listener->pin] < 2 ) 
        {
          debounce_cnt[listener->pin]++;
        }
        else
        {
          listener->status = 0;  //go to pressed state
          debounce_cnt[listener->pin] = 0; //reset counter
          
          if( listener->pin > 2 ) // ignore butons falling edge event
            xQueueSend(pinEventQueue, &listener->fallingEvent, portMAX_DELAY);
        }
      }
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
