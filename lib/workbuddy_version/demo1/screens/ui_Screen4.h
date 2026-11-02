// 阈值设置界面

#ifndef UI_SCREEN4_H
#define UI_SCREEN4_H

#ifdef __cplusplus
extern "C" {
#endif

/* 阈值条目编号 */
#define THR_VOLT_MAX   0
#define THR_VOLT_MIN   1
#define THR_CURR_MAX   2
#define THR_POWER_MAX  3
#define THR_TEMP_MAX   4
#define THR_HUM_MAX    5
#define THR_SMOKE_MAX  6
#define THR_VIB_MAX    7
#define THR_CNT        8

extern void ui_Screen4_screen_init(void);
extern void ui_Screen4_screen_destroy(void);
extern lv_obj_t * ui_Screen4;

/* 条目默认值与量程，main.c 的判异逻辑共用 */
extern const int32_t ui_thr_def[THR_CNT];
extern const int32_t ui_thr_min[THR_CNT];
extern const int32_t ui_thr_max[THR_CNT];

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
