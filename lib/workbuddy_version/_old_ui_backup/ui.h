// 工程头文件：字体声明、界面头文件、UI 与业务之间的接口
// 手写改造过，重新导出 SquareLine 工程时注意保留 FONTS / APP HOOK 两段。

#ifndef _SQUARELINE_PROJECT_UI_H
#define _SQUARELINE_PROJECT_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

#include "ui_common.h"
#include "ui_helpers.h"
#include "ui_events.h"

///////////////////// SCREENS ////////////////////

#include "screens/ui_Screen1.h"
#include "screens/ui_Screen2.h"
#include "screens/ui_Screen3.h"
#include "screens/ui_Screen4.h"

///////////////////// VARIABLES ////////////////////

// EVENTS
extern lv_obj_t * ui____initial_actions0;

///////////////////// FONTS ////////////////////
// 由 tools/gen_ui_font.py 从 STFANGSO.TTF 生成，覆盖界面上出现的全部汉字。
// 改文案后加字：往脚本的 SYMBOLS 里补字再跑一次。

LV_FONT_DECLARE(ui_font_cn16);      /* 小字：单位、标签         */
LV_FONT_DECLARE(ui_font_cn24);      /* 正文：参数名、数值       */
LV_FONT_DECLARE(ui_font_cn32);      /* 标题                     */

#define UI_FONT_SMALL  (&ui_font_cn16)
#define UI_FONT_NORMAL (&ui_font_cn24)
#define UI_FONT_TITLE  (&ui_font_cn32)

///////////////////// 数据快照 ////////////////////
// main.c 采集 -> 界面刷新，UI 层只读，不反向修改。

typedef struct {
    float volt;      /* 电压 V        */
    float curr;      /* 电流 A        */
    float power;     /* 功率 W        */
    float vib;       /* 振动强度 mm/s */
    bool  volt_al;   /* 逐项超限标记，用于标红 */
    bool  curr_al;
    bool  power_al;
    bool  vib_al;
} ui_dev_t;

typedef struct {
    float temp;      /* 温度 ℃      */
    float hum;       /* 湿度 %      */
    float smoke;     /* 烟雾浓度 ppm */
    bool  temp_al;
    bool  hum_al;
    bool  smoke_al;
} ui_env_t;

typedef struct {
    ui_dev_t motor;
    ui_dev_t pump;
    ui_env_t env;
    bool     motor_run;
    bool     pump_run;
    bool     motor_alarm;    /* 由阈值判定 */
    bool     pump_alarm;
    bool     env_alarm;
} ui_snap_t;

///////////////////// APP HOOK ////////////////////
// 界面只负责画，动作全部回调给 main.c 处理。

void app_nav_to(int idx);                 /* 底部快捷入口切换界面 */
void app_cloud_set(bool on);              /* 顶栏“数据上传”开关   */
void app_hist_channel(int idx, bool on);  /* 历史曲线通道显示/隐藏 */
void app_hist_source(int dev);            /* 历史曲线数据源 0=电机 1=水泵 */
void app_hist_clear(void);                /* 清空历史曲线 */
void app_camera_cmd(int cmd);             /* 摄像头按钮 0=拍照 1=录像 2=切换 */
void app_thr_set(int id, int32_t val);    /* 阈值滑块 */
void app_thr_save(void);
void app_thr_reset(void);

///////////////////// 界面刷新 ////////////////////

void ui_screen1_update(const ui_snap_t * s);
void ui_screen2_update(const char * time_str, bool online);
void ui_screen3_update(const ui_snap_t * s);
void ui_screen4_sync(const int32_t * vals, int cnt);   /* 阈值复位后刷新滑块 */

// UI INIT
void ui_init(void);
void ui_destroy(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
