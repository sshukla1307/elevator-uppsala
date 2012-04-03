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

#define GPIO_CALL_BUTTON 	(GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2)
#define GPIO_STOP_BUTTON 	GPIO_Pin_3

static void pollPin(PinListener *listener,
                    xQueueHandle pinEventQueue) {

  u8 data;

  //read GPIO Pin every 10 ms
  data = GPIO_ReadInputDataBit( listener->gpio, listener->pin );

	if (listener->pin & (GPIO_CALL_BUTTON | GPIO_STOP_BUTTON))	{
	  //debounce only for buttons
		switch (listener->status) {
	    case RELEASED: // released
	      if( data == 1 ) {
					listener->status++;        
				}
	      break;
	    case (RELEASED + 1): // first down
	      if( data == 1 ) {
					listener->status++;        
				}	else {
					listener->status = BOUNCED_RELEASED;	 //bounced while released
				}
	      break;
	    case (RELEASED + 2): // second down
	      if( data == 1 ) {
					listener->status = PRESSED; //pressed
					xQueueSend(pinEventQueue, &listener->risingEvent, portMAX_DELAY);        
				}	else {
					listener->status = INPUT_UNSTABLE; //env 4 violated
				}
	      break;
	    case BOUNCED_RELEASED: // bounced while released first down
	      if( data == 1 ) {
					listener->status++;         
				}	else {
					listener->status = INPUT_UNSTABLE; //env 4 violated
				}
	      break;
	    case (BOUNCED_RELEASED + 1): // bounced while released second down
	      if( data == 1 ) {
					listener->status = PRESSED; //pressed
					xQueueSend(pinEventQueue, &listener->risingEvent, portMAX_DELAY);        
				}	else {
					listener->status = INPUT_UNSTABLE; //env 4 violated
				}
	      break;	
	
	    case PRESSED: // pressed
	      if( data == 0 ) {
					listener->status++;        
				}
	      break;
	    case (PRESSED + 1): // first up
	      if( data == 0 ) {
					listener->status++;        
				}	else {
					listener->status = BOUNCED_PRESSED;	 //debounced while pressed
				}
	      break;
	    case (PRESSED + 2): // second up
	      if( data == 0 ) {
					listener->status = RELEASED; //released
					if (!(listener->pin & GPIO_CALL_BUTTON))	{// ignore falling edge event for the inputs from buttons
						xQueueSend(pinEventQueue, &listener->fallingEvent, portMAX_DELAY); 
					}         
				}	else {
					listener->status = INPUT_UNSTABLE; //env 4 violated
				}
	      break;
	    case BOUNCED_PRESSED: // bounced while pressed first up
	      if( data == 0 ) {
					listener->status++;         
				}	else {
					listener->status = INPUT_UNSTABLE; //env 4 violated
				}
	      break;
	    case (BOUNCED_PRESSED + 1): // bounced while pressed second up
	      if( data == 0 ) {
					listener->status = RELEASED; //released
					if (!(listener->pin & GPIO_CALL_BUTTON))	{// ignore falling edge event for the inputs from buttons
						xQueueSend(pinEventQueue, &listener->fallingEvent, portMAX_DELAY); 
					}        
				}	else {
					listener->status = INPUT_UNSTABLE; //env 4 violated
				}
	      break;		
	
			case INPUT_UNSTABLE: //input behaved incorrectly - will be noticed by the Safety module
				//do nothing
				break;
	
	    default:
	      break;
	
	  }
	} else {
		switch (listener->status) {
	    case (RELEASED): 
	      if( data == 1 ) {	//rising edge
					listener->status = PRESSED; //pressed
					xQueueSend(pinEventQueue, &listener->risingEvent, portMAX_DELAY);        
				}
	      break;
	    case (PRESSED): 
	      if( data == 0 ) {	//falling edge
					listener->status = RELEASED; //released
					if (!(listener->pin & GPIO_CALL_BUTTON))	{// ignore falling edge event for the inputs from buttons
						xQueueSend(pinEventQueue, &listener->fallingEvent, portMAX_DELAY); 
					}       
				}
	      break;			
		default:
	      break;
		}
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
