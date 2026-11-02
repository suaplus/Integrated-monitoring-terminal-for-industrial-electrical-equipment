// 摄像头画面界面

#ifndef UI_SCREEN2_H
#define UI_SCREEN2_H

#ifdef __cplusplus
extern "C" {
#endif

extern void ui_Screen2_screen_init(void);
extern void ui_Screen2_screen_destroy(void);
extern lv_obj_t * ui_Screen2;
extern void ui_screen2_set_source(int idx);   /* 切换摄像头后更新机位名 */

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
