#include "gui.h"
#include "tm_stm32f4_fatfs.h"
#include "stdio.h"

FRESULT scan_files (
    char* path        /* Start node to be scanned (also used as work area) */
)
{
    FRESULT res;
    FILINFO fno;
    DIR dir;
    int i;
    uint16_t line = 0;
    char *fn;   /* This function assumes non-Unicode configuration */
    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK) {
        i = strlen(path);
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
            if (fno.fname[0] == '.') continue;             /* Ignore dot entry */
            if (fno.fattrib & AM_DIR) {                    /* It is a directory */
                sprintf(&path[i], "/%s", fn);
                res = scan_files(path);
                path[i] = 0;
                if (res != FR_OK) break;
            } else {                                       /* It is a file. */
                LCD_DisplayStringLine(line++,(uint8_t*)fn);
                line %= 20;
            }
        }
        f_closedir(&dir);
    }

    return res;
}


void play_screen(){

}

void gui_start(){
    int set = 0;
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

    while(1){

        while (STM_EVAL_PBGetState(BUTTON_USER) != Bit_RESET)
        {
        }
        /* Wait for User push-button is released */
        while (STM_EVAL_PBGetState(BUTTON_USER) != Bit_SET)
        {
        }
        scan_files("/");
    }
    
}
