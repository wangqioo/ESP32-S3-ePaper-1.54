#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <esp_timer.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_log.h>
#include "lvgl.h"
#include "port_display.h"
#include "port_lvgl.h"

#include "user_app.h"


static void Lvgl_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * color_p) {
  	uint16_t *buffer = (uint16_t *)color_p;
    EPD_Clear();
  	for(int y = area->y1; y <= area->y2; y++) {
  	 	for(int x = area->x1; x <= area->x2; x++) {
  	 	   	uint8_t color = (*buffer < 0x7fff) ? DRIVER_COLOR_BLACK : DRIVER_COLOR_WHITE;
  	 	   	EPD_DrawColorPixel(x,y,color);
  	 	   	buffer++;
  	 	}
  	}
  	EPD_DisplayPart();
	lv_disp_flush_ready(disp);
}

extern "C" void app_main(void) {
	UserApp_Init();
    PortLvgl_Start_Init();
    Lvgl_PortInit(Lvgl_flush_cb);
    if(Lvgl_lock(-1)) {
        UserUi_Init();
        Lvgl_unlock();
    }
	UserApp_Start_Init();
}
