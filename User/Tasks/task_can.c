/*
	处理CAN总线相关的数据发送和接收。

*/

#include "task_can.h"
#include "cmsis_os.h"

#include "io.h"

#define CAN_TASK_FREQ_HZ (50)
#define CAN_TASK_STATUS_LED LED1

void Task_CAN(const void* argument) {
	uint32_t last_tick = osKernelSysTick();
	uint32_t delay_tick = 1000U / CAN_TASK_FREQ_HZ;
	
	LED_Set(CAN_TASK_STATUS_LED, LED_ON);
	//CNA_inti(task_handle);
	
	while(1) {
		
		
		
		LED_Set(CAN_TASK_STATUS_LED, LED_TAGGLE);
		osDelayUntil(delay_tick);
	}
}
