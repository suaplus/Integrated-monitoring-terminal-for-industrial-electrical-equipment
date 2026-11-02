// 界面公共实现：顶栏、底部快捷入口、通用卡片与数值格
// 手写部分（非 SquareLine 生成）。

#include "ui.h"
#include "ui_common.h"

/*---------------------------------------------------------------- 全局对象 */

/* [界面][入口] —— 四个界面各有一套导航按钮，切换时统一刷新高亮 */
lv_obj_t * ui_NavBtn[UI_SCR_CNT][UI_SCR_CNT];

static lv_obj_t * g_cloud_sw[UI_SCR_CNT];
static lv_obj_t * g_cloud_txt[UI_SCR_CNT];
static lv_obj_t * g_time_txt[UI_SCR_CNT];

static const char * g_nav_name[UI_SCR_CNT] = {"主界面", "摄像头", "历史数据", "阈值设置"};

/*---------------------------------------------------------------- 基础控件 */

lv_obj_t * ui_card(lv_obj_t * par, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t * c = lv_obj_create(par);
    lv_obj_remove_style_all(c);
    lv_obj_set_pos(c, x, y);
    lv_obj_set_size(c, w, h);
    lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(c, lv_color_hex(C_PANEL), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(c, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(c, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(c, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(c, lv_color_hex(C_BORDER), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(c, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    return c;
}

lv_obj_t * ui_text(lv_obj_t * par, const char * txt, const lv_font_t * font, uint32_t color,
                   lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_text_align_t align)
{
    lv_obj_t * l = lv_label_create(par);
    lv_obj_set_pos(l, x, y);
    lv_obj_set_width(l, w);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_font(l, font, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(l, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(l, align, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(l, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    return l;
}

void ui_cell_init(ui_cell_t * c, lv_obj_t * par, lv_coord_t x, lv_coord_t y,
                  lv_coord_t w, lv_coord_t h, const char * title, bool dot_en)
{
    c->box = lv_obj_create(par);
    lv_obj_remove_style_all(c->box);
    lv_obj_set_pos(c->box, x, y);
    lv_obj_set_size(c->box, w, h);
    lv_obj_clear_flag(c->box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(c->box, lv_color_hex(C_PANEL_ALT), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(c->box, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(c->box, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(c->box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* 标题 */
    c->title = ui_text(c->box, title, &ui_font_cn16, C_TEXT_DIM, 12, 8, w - 24, LV_TEXT_ALIGN_LEFT);

    /* 数值：24px，单行，超出自动截断 */
    c->value = ui_text(c->box, "--", &ui_font_cn24, C_TEXT, 12, 30, w - 24, LV_TEXT_ALIGN_LEFT);
    lv_label_set_long_mode(c->value, LV_LABEL_LONG_CLIP);

    /* 状态圆点 */
    c->dot = NULL;
    if(dot_en) {
        c->dot = lv_obj_create(c->box);
        lv_obj_remove_style_all(c->dot);
        lv_obj_set_pos(c->dot, w - 22, 10);
        lv_obj_set_size(c->dot, 10, 10);
        lv_obj_clear_flag(c->dot, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(c->dot, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(c->dot, lv_color_hex(C_IDLE), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(c->dot, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

/* 未使用 dot 的格子单独给个设颜色的接口，避免调用方碰内部成员 */
void ui_cell_set_dot(ui_cell_t * c, uint32_t color)
{
    if(c == NULL || c->dot == NULL) return;
    lv_obj_set_style_bg_color(c->dot, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
}

/*---------------------------------------------------------------- 顶栏 */

static void cloud_sw_event_cb(lv_event_t * e)
{
    lv_obj_t * sw = lv_event_get_target(e);
    if(lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    /* 具体动作（是否真的往云上发）在 main.c 里实现 */
    app_cloud_set(lv_obj_has_state(sw, LV_STATE_CHECKED));
}

void ui_header_create(lv_obj_t * par, int scr)
{
    lv_obj_t * bar = lv_obj_create(par);
    lv_obj_remove_style_all(bar);
    lv_obj_set_pos(bar, 0, 0);
    lv_obj_set_size(bar, UI_HOR_RES, UI_HEADER_H);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(bar, lv_color_hex(C_HEADER), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_text(bar, "工业用电设备监测终端", &ui_font_cn32, C_TEXT_INV, 16, 12, 320, LV_TEXT_ALIGN_LEFT);

    g_time_txt[scr] = ui_text(bar, "--:--", &ui_font_cn16, 0xB8C6D9, 348, 20, 160, LV_TEXT_ALIGN_LEFT);

    /* 数据上传云服务器开关 */
    ui_text(bar, "数据上传", &ui_font_cn16, 0xB8C6D9, 556, 20, 72, LV_TEXT_ALIGN_LEFT);

    lv_obj_t * sw = lv_switch_create(bar);
    lv_obj_set_pos(sw, 630, 15);
    lv_obj_set_size(sw, 52, 26);
    lv_obj_clear_flag(sw, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(sw, lv_color_hex(C_ACCENT), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(sw, lv_color_hex(0x5A6B80), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(sw, lv_color_hex(0xFFFFFF), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(sw, cloud_sw_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    g_cloud_sw[scr] = sw;

    g_cloud_txt[scr] = ui_text(bar, "已关闭", &ui_font_cn16, 0xB8C6D9, 692, 20, 56, LV_TEXT_ALIGN_LEFT);
}

void ui_header_set_cloud(bool on)
{
    int i;
    for(i = 0; i < UI_SCR_CNT; i++) {
        if(g_cloud_sw[i] == NULL) continue;
        if(on) lv_obj_add_state(g_cloud_sw[i], LV_STATE_CHECKED);
        else   lv_obj_clear_state(g_cloud_sw[i], LV_STATE_CHECKED);

        if(g_cloud_txt[i] != NULL) {
            lv_label_set_text(g_cloud_txt[i], on ? "已开启" : "已关闭");
            lv_obj_set_style_text_color(g_cloud_txt[i],
                                        lv_color_hex(on ? C_OK : 0xB8C6D9),
                                        LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
}

void ui_header_set_time(const char * txt)
{
    int i;
    for(i = 0; i < UI_SCR_CNT; i++) {
        if(g_time_txt[i] != NULL) lv_label_set_text(g_time_txt[i], txt);
    }
}

/*---------------------------------------------------------------- 底部快捷入口 */

static void nav_btn_event_cb(lv_event_t * e)
{
    int idx = (int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e));
    app_nav_to(idx);
}

void ui_navbar_create(lv_obj_t * par, int scr)
{
    lv_obj_t * bar;
    int i;
    const lv_coord_t bw = 170;
    const lv_coord_t bh = 54;
    const lv_coord_t gap = 12;
    lv_coord_t x0 = (UI_HOR_RES - (bw * UI_SCR_CNT + gap * (UI_SCR_CNT - 1))) / 2;

    bar = lv_obj_create(par);
    lv_obj_remove_style_all(bar);
    lv_obj_set_pos(bar, 0, UI_VER_RES - UI_NAV_H);
    lv_obj_set_size(bar, UI_HOR_RES, UI_NAV_H);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(bar, lv_color_hex(C_PANEL), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(bar, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(bar, LV_BORDER_SIDE_TOP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(bar, lv_color_hex(C_BORDER), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    for(i = 0; i < UI_SCR_CNT; i++) {
        lv_obj_t * btn = lv_btn_create(bar);
        lv_obj_remove_style_all(btn);
        lv_obj_set_pos(btn, x0 + i * (bw + gap), (UI_NAV_H - bh) / 2);
        lv_obj_set_size(btn, bw, bh);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_style_radius(btn, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(btn, lv_color_hex(C_NAV_OFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(btn, lv_color_hex(C_ACCENT), LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_user_data(btn, (void *)(intptr_t)i);
        lv_obj_add_event_cb(btn, nav_btn_event_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t * lb = lv_label_create(btn);
        lv_label_set_text(lb, g_nav_name[i]);
        lv_obj_set_style_text_font(lb, &ui_font_cn24, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(lb, lv_color_hex(C_TEXT), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(lb, lv_color_hex(C_TEXT_INV), LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_center(lb);

        ui_NavBtn[scr][i] = btn;
    }
}

void ui_nav_set_active(int idx)
{
    int s, i;
    for(s = 0; s < UI_SCR_CNT; s++) {
        for(i = 0; i < UI_SCR_CNT; i++) {
            lv_obj_t * btn = ui_NavBtn[s][i];
            if(btn == NULL) continue;
            if(i == idx) lv_obj_add_state(btn, LV_STATE_CHECKED);
            else         lv_obj_clear_state(btn, LV_STATE_CHECKED);
        }
    }
}
