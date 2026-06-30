#ifndef FACTEST_UI
#define FACTEST_UI
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

typedef struct
{
	lv_obj_t *screen;
	bool screen_del;
	lv_obj_t *screen_cont_1;
	lv_obj_t *screen_label_2;
	lv_obj_t *screen_label_1;
	lv_obj_t *screen_label_20;
	lv_obj_t *screen_label_5;
	lv_obj_t *screen_img_1;
	lv_obj_t *screen_label_13;
	lv_obj_t *screen_label_11;
	lv_obj_t *screen_img_2;
	lv_obj_t *screen_label_3;
	lv_obj_t *screen_label_4;
	lv_obj_t *screen_img_3;
	lv_obj_t *screen_label_8;
	lv_obj_t *screen_label_9;
	lv_obj_t *screen_img_4;
	lv_obj_t *screen_cont_2;
	lv_obj_t *screen_img_5;
	lv_obj_t *screen_cont_3;
	lv_obj_t *screen_label_23;
	lv_obj_t *screen_btn_4;
	lv_obj_t *screen_btn_4_label;
	lv_obj_t *screen_btn_3;
	lv_obj_t *screen_btn_3_label;
	lv_obj_t *screen_btn_2;
	lv_obj_t *screen_btn_2_label;
	lv_obj_t *screen_btn_1;
	lv_obj_t *screen_btn_1_label;
	lv_obj_t *screen_cont_4;
}lv_factest_ui;

void setup_factest_ui(lv_factest_ui *ui);

LV_IMAGE_DECLARE(_shidu_RGB565A8_20x20);
LV_IMAGE_DECLARE(_wendu_RGB565A8_20x20);
LV_IMAGE_DECLARE(_battery_RGB565A8_20x20);
LV_IMAGE_DECLARE(_3_RGB565A8_200x200);

LV_FONT_DECLARE(lv_font_montserratMedium_67)
LV_FONT_DECLARE(lv_font_montserratMedium_16)
LV_FONT_DECLARE(lv_font_montserratMedium_20)
LV_FONT_DECLARE(lv_font_MISANSREGULAR_20)
LV_FONT_DECLARE(lv_font_montserratMedium_17)
LV_FONT_DECLARE(lv_font_montserratMedium_12)


#ifdef __cplusplus
}
#endif

#endif