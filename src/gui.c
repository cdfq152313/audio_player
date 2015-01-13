#include "gui.h"
#include "tm_stm32f4_fatfs.h"
#include "stdio.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "pwm.h"

#define MAX_LINE 15

static uint8_t state = 1;
static uint8_t press_count = 0;
static uint8_t press_count_max = 5;
static uint8_t curfile = 0;
static uint8_t maxfile = 0;
static FILINFO file[MAX_LINE];
extern volatile xQueueHandle play_queue;

void gui_init(){
    /* LCD initialization */
    LCD_Init();
    LCD_LayerInit();

    /* LTDC reload configuration */  
    LTDC_ReloadConfig(LTDC_IMReload);

    /* Enable the LTDC */
    LTDC_Cmd(ENABLE);

    /* Set LCD foreground layer */
    LCD_SetLayer(LCD_FOREGROUND_LAYER);

    /* Initialize User Button mounted on STM32F429I-DISCO */
    STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_GPIO);

    /* Display test name on LCD */
    LCD_Clear(LCD_COLOR_WHITE);
    LCD_SetBackColor(LCD_COLOR_WHITE);
    LCD_SetTextColor(LCD_COLOR_BLACK);
}

void display_choosen_line(uint16_t line, uint8_t* str){
    /* Display test name on LCD */
    LCD_SetBackColor(LCD_COLOR_BLUE);
    LCD_SetTextColor(LCD_COLOR_GREY);
    LCD_ClearLine( LINE(line) );
    LCD_DisplayStringLine(LINE(line), str);
}

void display_normal_line(uint16_t line, uint8_t* str){
    LCD_SetBackColor(LCD_COLOR_WHITE);
    LCD_SetTextColor(LCD_COLOR_BLACK);
    LCD_ClearLine( LINE(line) );
    LCD_DisplayStringLine(LINE(line), str);
}


FRESULT scan_files (char* path)
{
    FRESULT res;
    DIR dir;
    uint16_t index = 0;

    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK) {
        while (index < MAX_LINE) {
            res = f_readdir(&dir, &file[index]);                   /* Read a directory item */
            if (res != FR_OK || file[index].fname[0] == 0) break;  /* Break on error or end of dir */
            if (file[index].fname[0] == '.') continue;             /* Ignore dot entry */
            if(index == 0)
                display_choosen_line(index, file[index].fname);
            else
                display_normal_line(index, file[index].fname);
            index ++;
        }
        maxfile = index;
        f_closedir(&dir);
    }
    return res;
}

void release_button(){
    press_count = 0;
    state = 1;
}

void gui_start(void *pvParameters){
    portBASE_TYPE xStatus;
    scan_files("/");

    while(1){
        switch(state){
        case 1:
            if (STM_EVAL_PBGetState(BUTTON_USER) != Bit_RESET){
                press_count ++;
                state = 2;
            }
            if (STM_EVAL_PBGetState(BUTTON_USER) != Bit_SET){
                release_button();
            }
        break;
        case 2:
            if (STM_EVAL_PBGetState(BUTTON_USER) != Bit_RESET){
                press_count ++;
                if(press_count >= press_count_max)
                    state = 3;
            }
            if (STM_EVAL_PBGetState(BUTTON_USER) != Bit_SET){
                display_normal_line(curfile, file[curfile].fname);
                curfile ++;
                curfile %= maxfile;
                display_choosen_line(curfile, file[curfile].fname);
                release_button();
            }
        break;
        case 3:
            if (STM_EVAL_PBGetState(BUTTON_USER) != Bit_RESET){
                xStatus = xQueueSendToBack( play_queue, &file[curfile], ( TickType_t ) 10 );
                if( xStatus != pdPASS )
                {
                    LCD_DisplayStringLine(LINE(4),(uint8_t*)"send error");
                }
                state = 4;
            }
            if (STM_EVAL_PBGetState(BUTTON_USER) != Bit_SET)
                release_button();
        break;
        case 4:
            if (STM_EVAL_PBGetState(BUTTON_USER) != Bit_SET)
                release_button();
        break;
        }
        vTaskDelay(50);
    }
}

void gui_play(void *pvParameters){
    portBASE_TYPE xStatus;
    FILINFO file;
    while(1){
        xStatus = xQueueReceive( play_queue, &file, portMAX_DELAY );
        if( xStatus == pdPASS )
        {
            LCD_ClearLine(LINE(5));
            LCD_DisplayStringLine(LINE(5),(uint8_t*)file.fname);
            play(file.fname);
        }
        else{
            LCD_ClearLine(LINE(5));
            LCD_DisplayStringLine(LINE(5),(uint8_t*)"could not receive data");
        }
    }
}
