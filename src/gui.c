#include "gui.h"
#include "tm_stm32f4_fatfs.h"
#include "stdio.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

FRESULT scan_files (char* path )
{
    FRESULT res;
    FILINFO fno;
    DIR dir;
    int i;
    uint16_t line = 0;
    char *fn;   /* This function assumes non-Unicode configuration */

    res = f_opendir(&dir, path);                       /* Open the directory */
    LCD_DisplayStringLine(LCD_LINE_0,(uint8_t*)"open d ok");
    if (res == FR_OK) {
        i = strlen(path);
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            LCD_DisplayStringLine(LCD_LINE_0,(uint8_t*)"read d ok");
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
            if (fno.fname[0] == '.') continue;             /* Ignore dot entry */
            fn = fno.fname;
            if (fno.fattrib & AM_DIR) {                    /* It is a directory */
                sprintf(&path[i], "/%s", fn);
                res = scan_files(path);
                path[i] = 0;
                if (res != FR_OK) break;
            } else {                                       /* It is a file. */
                LCD_DisplayStringLine(LCD_LINE_1,(uint8_t*)fn);
                line %= 20;
                LCD_DisplayStringLine(LCD_LINE_2,(uint8_t*)"press button");

                while (STM_EVAL_PBGetState(BUTTON_USER) != Bit_RESET)
                {
                }
                /* Wait for User push-button is released */
                while (STM_EVAL_PBGetState(BUTTON_USER) != Bit_SET)
                {
                }
            }
        }
        f_closedir(&dir);
    }

    return res;
}


void play_screen(){

}

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
    LCD_SetBackColor(LCD_COLOR_BLUE);
    LCD_SetTextColor(LCD_COLOR_WHITE);
    LCD_DisplayStringLine(LCD_LINE_4,(uint8_t*)"Hello world");
    LCD_DisplayStringLine(LCD_LINE_5,(uint8_t*)"Press User button");
    LCD_DisplayStringLine(LCD_LINE_6,(uint8_t*)"to display file");

}

void gui_start1(void *pvParameters){
    int set = 0;
    while(1){
        LCD_ClearLine(LCD_LINE_7);
        LCD_ClearLine(LCD_LINE_8);
        if(set){
            LCD_DisplayStringLine(LCD_LINE_7,(uint8_t*)"task oneee");
            set = !set;
        }
        else{
            LCD_DisplayStringLine(LCD_LINE_8,(uint8_t*)"task oneee");
            set = !set;
        }
        vTaskDelay(500);
    }
}

void gui_start2(void *pvParameters){
    int set = 0;
    while(1){
        LCD_ClearLine(LCD_LINE_9);
        LCD_ClearLine(LCD_LINE_10);
        if(set){
            LCD_DisplayStringLine(LCD_LINE_9,(uint8_t*)"task twoo");
            set = !set;
        }
        else{
            LCD_DisplayStringLine(LCD_LINE_10,(uint8_t*)"task twoo");
            set = !set;
        }
        vTaskDelay(500);
    }
}