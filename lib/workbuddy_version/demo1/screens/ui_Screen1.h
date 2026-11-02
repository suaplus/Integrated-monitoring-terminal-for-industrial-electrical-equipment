// 主界面：电机 / 水泵运行状态 + 电气参数 + 环境参数

#ifndef UI_SCREEN1_H
#define UI_SCREEN1_H

#ifdef __cplusplus
extern "C" {
#endif

extern void ui_Screen1_screen_init(void);
extern void ui_Screen1_screen_destroy(void);
extern lv_obj_t * ui_Screen1;

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
