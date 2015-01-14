#include "defines.h"
#include "tm_stm32f4_delay.h"
#include "tm_stm32f4_spi.h"
#include "tm_stm32f4_fatfs.h"
#include <stdio.h>
#include <string.h>

#include "stm32_init.h"
#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <string.h>

#include "host.h"

#include "gui.h"
#include "pwm.h"
//static void setup_hardware();

volatile xSemaphoreHandle serial_tx_wait_sem = NULL;
/* Add for serial input */
volatile xQueueHandle serial_rx_queue = NULL;

volatile xSemaphoreHandle play_sem = NULL;
volatile xQueueHandle play_queue = NULL;

FATFS FatFs;


int main()
{
	RCC_Configuration();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    gui_init();
    player_init();

    vSemaphoreCreateBinary(play_sem);
    play_queue = xQueueCreate(1, sizeof(FILINFO));
    if (f_mount(&FatFs, "/", 1) != FR_OK) {
        LCD_DisplayStringLine(LCD_LINE_7,(uint8_t*)"mount error QAQ");
        while(1);
    }

    xTaskCreate(gui_start,
                (signed portCHAR *) "start",
                128 /* stack size */, NULL, tskIDLE_PRIORITY + 2, NULL);

    // xTaskCreate(gui_play,
    //             (signed portCHAR *) "play",
    //             128 /* stack size */, NULL, tskIDLE_PRIORITY + 2, NULL);

	// /* Start running the tasks. */
	vTaskStartScheduler();

	return 0;
}

void vApplicationTickHook()
{
}
