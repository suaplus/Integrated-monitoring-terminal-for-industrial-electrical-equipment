// 第四屏：阈值设置
//
// 电压上下限、电流 / 功率 / 振动强度上限，以及温度 / 湿度 / 烟雾浓度上限。
// 拖动滑块即时生效到内存里的阈值表，点“保存”落盘，点“恢复默认”回到出厂值。

#include "../ui.h"
#include "../ui_common.h"

#include <stdio.h>
#include <string.h>

#define CELL_W  384
#define CELL_H  64
#define CELL_GAP 8
#define CELL_X1 12
#define CELL_X2 404
#define CELL_Y0 68

lv_obj_t * ui_Screen4 = NULL;

/* 名称 / 单位 / 默认 / 最小 / 最大 */
static const char * g_name[THR_CNT] = {
    "电压上限", "电压下限", "电流上限", "功率上限",
    "温度上限", "湿度上限", "烟雾上限", "振动上限",
};
static const char * g_unit[THR_CNT] = {
    "V", "V", "A", "W", "℃", "%", "ppm", "mm/s",
};

const int32_t ui_thr_def[THR_CNT] = {400, 320, 20, 2200, 65, 80, 300, 8};
const int32_t ui_thr_min[THR_CNT] = {0,   0,   0,  0,    0,  0,  0,   0};
const int32_t ui_thr_max[THR_CNT] = {500, 500, 50, 3000, 120, 100, 1000, 50};

static lv_obj_t * g_slider[THR_CNT];
static lv_obj_t * g_value[THR_CNT];
static lv_obj_t * g_hint = NULL;

/*---------------------------------------------------------------- 事件 */

static void thr_slider_event_cb(lv_event_t * e)
{
    lv_obj_t * sl = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    int id = (int)(intptr_t)lv_obj_get_user_data(sl);
    int32_t v;
    char buf[24];

    if(id < 0 || id >= THR_CNT) return;
    if(code != LV_EVENT_VALUE_CHANGED && code != LV_EVENT_RELEASED) return;

    v = lv_slider_get_value(sl);
    snprintf(buf, sizeof(buf), "%ld %s", (long)v, g_unit[id]);
    lv_label_set_text(g_value[id], buf);

    /* 只有拖动结束才真正下发，避免拖动过程里疯狂刷新 */
    if(lv_event_get_code(e) == LV_EVENT_RELEASED) app_thr_set(id, v);
}

static void btn_event_cb(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_target(e);
    int cmd = (int)(intptr_t)lv_obj_get_user_data(btn);

    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    if(cmd == 0) app_thr_save();
    else         app_thr_reset();
}

/*---------------------------------------------------------------- 构件 */

static lv_obj_t * bottom_btn(lv_obj_t * par, const char * txt, lv_coord_t x, int cmd, bool primary)
{
    lv_obj_t * btn = lv_btn_create(par);
    lv_obj_remove_style_all(btn);
    lv_obj_set_pos(btn, x, 360);
    lv_obj_set_size(btn, 150, 44);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(btn, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(primary ? C_ACCENT : C_NAV_OFF),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_user_data(btn, (void *)(intptr_t)cmd);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * lb = lv_label_create(btn);
    lv_label_set_text(lb, txt);
    lv_obj_set_style_text_font(lb, &ui_font_cn24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lb, lv_color_hex(primary ? C_TEXT_INV : C_TEXT),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(lb);

    return btn;
}

/*---------------------------------------------------------------- 构建 */

void ui_Screen4_screen_init(void)
{
    int i;

    ui_Screen4 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen4, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen4, lv_color_hex(C_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen4, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_header_create(ui_Screen4, UI_SCR_THRESHOLD);

    /* 2 列 x 4 行 */
    for(i = 0; i < THR_CNT; i++) {
        lv_coord_t x = (i % 2) ? CELL_X2 : CELL_X1;
        lv_coord_t y = CELL_Y0 + (i / 2) * (CELL_H + CELL_GAP);

        lv_obj_t * cell = ui_card(ui_Screen4, x, y, CELL_W, CELL_H);

        ui_text(cell, g_name[i], &ui_font_cn16, C_TEXT, 12, 8, 160, LV_TEXT_ALIGN_LEFT);
        g_value[i] = ui_text(cell, "--", &ui_font_cn24, C_TEXT, 208, 4, 160, LV_TEXT_ALIGN_RIGHT);

        lv_obj_t * sl = lv_slider_create(cell);
        lv_obj_set_pos(sl, 12, 34);
        lv_obj_set_size(sl, CELL_W - 24, 18);
        lv_slider_set_range(sl, ui_thr_min[i], ui_thr_max[i]);
        lv_slider_set_value(sl, ui_thr_def[i], LV_ANIM_OFF);
        lv_obj_set_style_bg_color(sl, lv_color_hex(0xDCE2EA), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(sl, lv_color_hex(C_ACCENT), LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(sl, lv_color_hex(0xFFFFFF), LV_PART_KNOB | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(sl, 2, LV_PART_KNOB | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(sl, lv_color_hex(C_ACCENT), LV_PART_KNOB | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(sl, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
        lv_obj_set_user_data(sl, (void *)(intptr_t)i);
        lv_obj_add_event_cb(sl, thr_slider_event_cb, LV_EVENT_ALL, NULL);

        g_slider[i] = sl;
    }

    bottom_btn(ui_Screen4, "恢复默认", 240, 1, false);
    bottom_btn(ui_Screen4, "保存",     410, 0, true);

    g_hint = ui_text(ui_Screen4, "修改后点保存生效", &ui_font_cn16, C_TEXT_DIM,
                     578, 372, 200, LV_TEXT_ALIGN_LEFT);

    ui_navbar_create(ui_Screen4, UI_SCR_THRESHOLD);
}

void ui_Screen4_screen_destroy(void)
{
    int i;
    if(ui_Screen4) lv_obj_del(ui_Screen4);
    ui_Screen4 = NULL;
    g_hint = NULL;
    for(i = 0; i < THR_CNT; i++) {
        g_slider[i] = NULL;
        g_value[i]  = NULL;
    }
}

/*---------------------------------------------------------------- 刷新 */

/* 用一组值刷新滑块与数值（初始化 / 恢复默认时调用） */
void ui_screen4_sync(const int32_t * vals, int cnt)
{
    int i;
    char buf[24];

    if(ui_Screen4 == NULL || vals == NULL) return;

    for(i = 0; i < THR_CNT && i < cnt; i++) {
        int32_t v = vals[i];
        if(v < ui_thr_min[i]) v = ui_thr_min[i];
        if(v > ui_thr_max[i]) v = ui_thr_max[i];

        lv_slider_set_value(g_slider[i], v, LV_ANIM_OFF);
        snprintf(buf, sizeof(buf), "%ld %s", (long)v, g_unit[i]);
        lv_label_set_text(g_value[i], buf);
    }

    if(g_hint != NULL) {
        lv_label_set_text(g_hint, "修改后点保存生效");
        lv_obj_set_style_text_color(g_hint, lv_color_hex(C_TEXT_DIM), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

/* 保存反馈 */
void ui_screen4_hint(const char * txt, bool ok)
{
    if(ui_Screen4 == NULL || g_hint == NULL || txt == NULL) return;
    lv_label_set_text(g_hint, txt);
    lv_obj_set_style_text_color(g_hint, lv_color_hex(ok ? C_OK : C_ALARM),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
}
