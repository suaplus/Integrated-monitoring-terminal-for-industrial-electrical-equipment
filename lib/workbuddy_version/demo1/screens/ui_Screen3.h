// 历史数据折线图界面

#ifndef UI_SCREEN3_H
#define UI_SCREEN3_H

#ifdef __cplusplus
extern "C" {
#endif

/* 曲线采样点数：界面与 main.c 的历史环形缓冲必须一致 */
#define UI_HIST_POINTS  50

/* 通道编号：0~2 环境量，3~6 电气量（随数据源在电机/水泵之间切换） */
#define UI_HIST_TEMP    0
#define UI_HIST_HUM     1
#define UI_HIST_SMOKE   2
#define UI_HIST_VOLT    3
#define UI_HIST_CURR    4
#define UI_HIST_POWER   5
#define UI_HIST_VIB     6
#define UI_HIST_CNT     7

extern void ui_Screen3_screen_init(void);
extern void ui_Screen3_screen_destroy(void);
extern lv_obj_t * ui_Screen3;
extern lv_obj_t * ui_HistChart;

/* 七个通道的配色，曲线与图例共用 */
extern const uint32_t ui_hist_color[UI_HIST_CNT];
extern const char * ui_hist_name[UI_HIST_CNT];

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
