// 界面公共定义：分辨率、配色、顶栏、底部快捷入口、通用卡片/数值格
// 手写部分（非 SquareLine 生成），重新导出 SquareLine 工程时不要覆盖本文件。

#ifndef UI_COMMON_H
#define UI_COMMON_H

#include "lvgl/lvgl.h"

#include <stdbool.h>
#include <stdint.h>

/*---------------------------------------------------------------- 布局常量 */
#define UI_HOR_RES   800
#define UI_VER_RES   480

#define UI_HEADER_H  56                                  /* 顶部标题栏       */
#define UI_NAV_H     70                                  /* 底部快捷入口栏   */
#define UI_PAD       12                                  /* 内容区外边距     */

#define UI_BODY_Y    UI_HEADER_H
#define UI_BODY_H    (UI_VER_RES - UI_HEADER_H - UI_NAV_H)   /* 354 */
#define UI_BODY_X    UI_PAD
#define UI_BODY_W    (UI_HOR_RES - UI_PAD * 2)               /* 776 */

/*---------------------------------------------------------------- 配色 */
#define C_BG         0xF2F4F7   /* 背景             */
#define C_PANEL      0xFFFFFF   /* 卡片             */
#define C_PANEL_ALT  0xF7F9FC   /* 卡片内浅底       */
#define C_BORDER     0xE2E7EE   /* 描边             */
#define C_HEADER     0x14304F   /* 顶栏             */

#define C_TEXT       0x1B2A41   /* 主文字           */
#define C_TEXT_DIM   0x7A8798   /* 次要文字         */
#define C_TEXT_INV   0xFFFFFF   /* 深底反白文字     */

#define C_ACCENT     0x1668DC   /* 主题蓝           */
#define C_OK         0x18A058   /* 正常             */
#define C_WARN       0xE8912A   /* 预警             */
#define C_ALARM      0xD93636   /* 异常/报警        */
#define C_IDLE       0x9AA5B1   /* 停机/离线        */

#define C_MOTOR      0x2F6BFF   /* 电机主题色       */
#define C_PUMP       0x0FA3A3   /* 水泵主题色       */
#define C_ENV        0x7B61FF   /* 环境主题色       */
#define C_NAV_OFF    0xE9EDF2   /* 导航未选中       */

/*---------------------------------------------------------------- 界面编号 */
typedef enum {
    UI_SCR_HOME = 0,      /* 主界面   */
    UI_SCR_CAMERA,        /* 摄像头   */
    UI_SCR_HISTORY,       /* 历史数据 */
    UI_SCR_THRESHOLD,     /* 阈值设置 */
    UI_SCR_CNT
} ui_scr_id_t;

/*---------------------------------------------------------------- 数值格 */
/* 一个“标题 + 数值 + 状态圆点”的小格子，界面上大量复用 */
typedef struct {
    lv_obj_t * box;
    lv_obj_t * title;
    lv_obj_t * value;
    lv_obj_t * dot;      /* 允许为 NULL */
} ui_cell_t;

/*---------------------------------------------------------------- 通用控件 */
/* 白底圆角卡片 */
lv_obj_t * ui_card(lv_obj_t * par, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h);

/* 在 par 上摆一个数值格，dot_en=1 时右上角带状态圆点 */
void ui_cell_init(ui_cell_t * c, lv_obj_t * par, lv_coord_t x, lv_coord_t y,
                  lv_coord_t w, lv_coord_t h, const char * title, bool dot_en);

/* 右侧对齐的纯文本标签 */
lv_obj_t * ui_text(lv_obj_t * par, const char * txt, const lv_font_t * font, uint32_t color,
                   lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_text_align_t align);

/*---------------------------------------------------------------- 顶栏 / 底栏 */
/* 顶栏：标题 + 时间 + “数据上传云服务器”开关。每个界面各有一份。 */
void ui_header_create(lv_obj_t * par, int scr);
/* 底栏：四个界面快捷入口。每个界面各有一份。 */
void ui_navbar_create(lv_obj_t * par, int scr);

void ui_nav_set_active(int idx);
void ui_header_set_cloud(bool on);
void ui_header_set_time(const char * txt);

/* 四个界面的导航按钮 [界面][入口]，切换界面时统一刷新高亮 */
extern lv_obj_t * ui_NavBtn[UI_SCR_CNT][UI_SCR_CNT];

#endif /* UI_COMMON_H */
