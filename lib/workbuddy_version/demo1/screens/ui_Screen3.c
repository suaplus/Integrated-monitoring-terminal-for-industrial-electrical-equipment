// 第三屏：历史数据折线图
//
// 七个通道：温度 / 湿度 / 烟雾 / 电压 / 电流 / 功率 / 振动强度
// 纵坐标统一为“占各自量程的百分比”，这样温湿度和几百伏的电压能画在同一张图上；
// 真实数值显示在右侧“实时值”列里。
// 电压/电流/功率/振动四个电气通道可通过“数据源”在电机与水泵之间切换。

#include "../ui.h"
#include "../ui_common.h"

#include <stdio.h>
#include <string.h>

#define CHART_X   12
#define CHART_Y   68
#define CHART_W   536
#define CHART_H   250

#define LEGEND_Y  326
#define LEGEND_H  42
#define LEGEND_W  68
#define LEGEND_GAP 6

#define SIDE_X    560
#define SIDE_W    228
#define SIDE_H    342

lv_obj_t * ui_Screen3 = NULL;
lv_obj_t * ui_HistChart = NULL;

const char * ui_hist_name[UI_HIST_CNT] = {"温度", "湿度", "烟雾", "电压", "电流", "功率", "振动"};

const uint32_t ui_hist_color[UI_HIST_CNT] = {
    0xE53935,   /* 温度 红 */
    0x1E88E5,   /* 湿度 蓝 */
    0x43A047,   /* 烟雾 绿 */
    0xFB8C00,   /* 电压 橙 */
    0x8E24AA,   /* 电流 紫 */
    0x00897B,   /* 功率 青 */
    0xD81B60,   /* 振动 玫红 */
};

static lv_obj_t * g_legend[UI_HIST_CNT];
static lv_obj_t * g_legend_lb[UI_HIST_CNT];
static lv_obj_t * g_src_btn[2];
static lv_obj_t * g_src_lb[2];
static lv_obj_t * g_live[UI_HIST_CNT];

/*---------------------------------------------------------------- 事件 */

static void legend_event_cb(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    int idx = (int)(intptr_t)lv_obj_get_user_data(btn);

    if(idx < 0 || idx >= UI_HIST_CNT) return;

    if(code == LV_EVENT_CLICKED) {
        app_hist_channel(idx, lv_obj_has_state(btn, LV_STATE_CHECKED));
    }

    /* 子标签不会自动继承按钮的 checked，手动同步文字配色 */
    if(code == LV_EVENT_VALUE_CHANGED && g_legend_lb[idx] != NULL) {
        bool on = lv_obj_has_state(btn, LV_STATE_CHECKED);
        if(on) lv_obj_add_state(g_legend_lb[idx], LV_STATE_CHECKED);
        else   lv_obj_clear_state(g_legend_lb[idx], LV_STATE_CHECKED);
    }
}

static void src_event_cb(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_target(e);
    int dev = (int)(intptr_t)lv_obj_get_user_data(btn);
    int i;

    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    /* 电机 / 水泵 二选一（子标签的 checked 要手动同步） */
    for(i = 0; i < 2; i++) {
        bool on = (i == dev);
        if(g_src_btn[i] != NULL) {
            if(on) lv_obj_add_state(g_src_btn[i], LV_STATE_CHECKED);
            else   lv_obj_clear_state(g_src_btn[i], LV_STATE_CHECKED);
        }
        if(g_src_lb[i] != NULL) {
            if(on) lv_obj_add_state(g_src_lb[i], LV_STATE_CHECKED);
            else   lv_obj_clear_state(g_src_lb[i], LV_STATE_CHECKED);
        }
    }

    app_hist_source(dev);
}

static void clear_event_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    app_hist_clear();
}

/*---------------------------------------------------------------- 构件 */

static lv_obj_t * small_btn(lv_obj_t * par, const char * txt, lv_coord_t x, lv_coord_t y,
                            lv_coord_t w, lv_coord_t h, lv_event_cb_t cb, void * ud)
{
    lv_obj_t * btn = lv_btn_create(par);
    lv_obj_remove_style_all(btn);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, w, h);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_style_radius(btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(C_NAV_OFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_user_data(btn, ud);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_ALL, NULL);

    lv_obj_t * lb = lv_label_create(btn);
    lv_label_set_text(lb, txt);
    lv_obj_set_style_text_font(lb, &ui_font_cn16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lb, lv_color_hex(C_TEXT), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lb, lv_color_hex(C_TEXT_INV), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_center(lb);

    return btn;
}

/*---------------------------------------------------------------- 构建 */

void ui_Screen3_screen_init(void)
{
    lv_obj_t * panel, * side;
    int i;

    ui_Screen3 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen3, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen3, lv_color_hex(C_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen3, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_header_create(ui_Screen3, UI_SCR_HISTORY);

    /*---------------- 曲线 ----------------*/
    panel = ui_card(ui_Screen3, CHART_X, CHART_Y, CHART_W, CHART_H);

    ui_HistChart = lv_chart_create(panel);
    lv_obj_set_pos(ui_HistChart, 20, 40);
    lv_obj_set_size(ui_HistChart, CHART_W - 40, CHART_H - 56);
    lv_chart_set_type(ui_HistChart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(ui_HistChart, UI_HIST_POINTS);
    lv_chart_set_range(ui_HistChart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_div_line_count(ui_HistChart, 5, 6);
    lv_chart_set_axis_tick(ui_HistChart, LV_CHART_AXIS_PRIMARY_Y, 6, 3, 5, 2, true, 40);
    lv_obj_set_style_size(ui_HistChart, 0, LV_PART_INDICATOR);   /* 不画点标记，省绘制 */
    lv_obj_clear_flag(ui_HistChart, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_text_font(ui_HistChart, &ui_font_cn16, LV_PART_MAIN | LV_STATE_DEFAULT);

    /*---------------- 图例 / 通道开关 ----------------*/
    for(i = 0; i < UI_HIST_CNT; i++) {
        lv_obj_t * btn = small_btn(ui_Screen3, ui_hist_name[i],
                                   CHART_X + i * (LEGEND_W + LEGEND_GAP), LEGEND_Y,
                                   LEGEND_W, LEGEND_H, legend_event_cb, (void *)(intptr_t)i);
        lv_obj_set_style_bg_color(btn, lv_color_hex(ui_hist_color[i]),
                                  LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_add_state(btn, LV_STATE_CHECKED);              /* 默认全显示 */
        lv_obj_set_style_text_font(lv_obj_get_child(btn, 0), &ui_font_cn16,
                                   LV_PART_MAIN | LV_STATE_DEFAULT);
        g_legend[i]    = btn;
        g_legend_lb[i] = lv_obj_get_child(btn, 0);
        lv_obj_add_state(g_legend_lb[i], LV_STATE_CHECKED);
    }

    ui_text(ui_Screen3, "纵轴为各参数占量程百分比 · 最近 50 点",
            &ui_font_cn16, C_TEXT_DIM, CHART_X, 380, 360, LV_TEXT_ALIGN_LEFT);

    lv_obj_t * btn = lv_btn_create(ui_Screen3);
    lv_obj_remove_style_all(btn);
    lv_obj_set_pos(btn, CHART_X + 372, 374);
    lv_obj_set_size(btn, 128, 34);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(C_NAV_OFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn, clear_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lb = lv_label_create(btn);
    lv_label_set_text(lb, "清空");
    lv_obj_set_style_text_font(lb, &ui_font_cn16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lb, lv_color_hex(C_TEXT), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(lb);

    /*---------------- 右侧：数据源 + 实时值 ----------------*/
    side = ui_card(ui_Screen3, SIDE_X, CHART_Y, SIDE_W, SIDE_H);
    ui_text(side, "数据源", &ui_font_cn16, C_TEXT_DIM, 14, 12, 100, LV_TEXT_ALIGN_LEFT);

    g_src_btn[0] = small_btn(side, "电机", 14, 34, 95, 40, src_event_cb, (void *)(intptr_t)0);
    g_src_btn[1] = small_btn(side, "水泵", 119, 34, 95, 40, src_event_cb, (void *)(intptr_t)1);
    lv_obj_set_style_bg_color(g_src_btn[0], lv_color_hex(C_MOTOR), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(g_src_btn[1], lv_color_hex(C_PUMP), LV_PART_MAIN | LV_STATE_CHECKED);
    g_src_lb[0] = lv_obj_get_child(g_src_btn[0], 0);
    g_src_lb[1] = lv_obj_get_child(g_src_btn[1], 0);

    lv_obj_add_state(g_src_btn[0], LV_STATE_CHECKED);        /* 默认看电机 */
    lv_obj_add_state(g_src_lb[0], LV_STATE_CHECKED);

    ui_text(side, "实时值", &ui_font_cn24, C_TEXT, 14, 88, 120, LV_TEXT_ALIGN_LEFT);

    for(i = 0; i < UI_HIST_CNT; i++) {
        lv_coord_t y = 122 + i * 32;
        ui_text(side, ui_hist_name[i], &ui_font_cn16, C_TEXT_DIM, 14, y, 90, LV_TEXT_ALIGN_LEFT);
        g_live[i] = ui_text(side, "--", &ui_font_cn16, C_TEXT, 104, y, 110, LV_TEXT_ALIGN_RIGHT);
    }

    ui_navbar_create(ui_Screen3, UI_SCR_HISTORY);
}

void ui_Screen3_screen_destroy(void)
{
    int i;
    if(ui_Screen3) lv_obj_del(ui_Screen3);
    ui_Screen3   = NULL;
    ui_HistChart = NULL;
    for(i = 0; i < UI_HIST_CNT; i++) {
        g_legend[i]    = NULL;
        g_legend_lb[i] = NULL;
        g_live[i]      = NULL;
    }
    g_src_btn[0] = NULL;
    g_src_btn[1] = NULL;
    g_src_lb[0]  = NULL;
    g_src_lb[1]  = NULL;
}

/*---------------------------------------------------------------- 刷新 */

void ui_screen3_update(const ui_snap_t * s)
{
    const ui_dev_t * d;
    char buf[24];
    int i;
    float v[UI_HIST_CNT];
    const char * fmt[UI_HIST_CNT] = {"%.1f ℃", "%.1f %%", "%.0f ppm",
                                     "%.1f V", "%.1f A", "%.0f W", "%.1f mm/s"};

    if(ui_Screen3 == NULL || s == NULL) return;

    /* 电气量跟随“数据源”按钮当前选中的设备 */
    d = (g_src_btn[1] != NULL && lv_obj_has_state(g_src_btn[1], LV_STATE_CHECKED))
            ? &s->pump : &s->motor;

    v[UI_HIST_TEMP]  = s->env.temp;
    v[UI_HIST_HUM]   = s->env.hum;
    v[UI_HIST_SMOKE] = s->env.smoke;
    v[UI_HIST_VOLT]  = d->volt;
    v[UI_HIST_CURR]  = d->curr;
    v[UI_HIST_POWER] = d->power;
    v[UI_HIST_VIB]   = d->vib;

    for(i = 0; i < UI_HIST_CNT; i++) {
        if(g_live[i] == NULL) continue;
        snprintf(buf, sizeof(buf), fmt[i], (double)v[i]);
        lv_label_set_text(g_live[i], buf);
        lv_obj_set_style_text_color(g_live[i], lv_color_hex(ui_hist_color[i]),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}
