// 第二屏：摄像头画面
//
// 左侧是视频区（当前用 LVGL 图形模拟一路画面：机架 + 流水线上的移动目标 + 时间/REC 叠加层）。
// 接真实摄像头时：把 g_scene 里的模拟元素隐藏，改用 lv_img/canvas 显示解码后的帧即可，
// 叠加层（时间戳、REC、分辨率）不用改。

#include "../ui.h"
#include "../ui_common.h"

#include <stdio.h>
#include <string.h>

#define VID_X  12
#define VID_Y  68
#define VID_W  536
#define VID_H  342

#define SIDE_X 560
#define SIDE_W 228

lv_obj_t * ui_Screen2 = NULL;

static lv_obj_t * g_scene   = NULL;    /* 模拟画面容器   */
static lv_obj_t * g_nosig   = NULL;    /* “无视频信号”   */
static lv_obj_t * g_rec_dot = NULL;    /* REC 红点       */
static lv_obj_t * g_time_lb = NULL;    /* 叠加时间戳     */
static lv_obj_t * g_state_v = NULL;    /* 侧栏“状态”值   */
static lv_obj_t * g_btn_rec = NULL;    /* 录像按钮       */
static lv_obj_t * g_rec_lb  = NULL;    /* 录像按钮文字   */
static lv_obj_t * g_src_lb  = NULL;    /* 左上角机位名   */

/*---------------------------------------------------------------- 事件 */

static void cam_btn_event_cb(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    int cmd = (int)(intptr_t)lv_obj_get_user_data(btn);

    if(code == LV_EVENT_CLICKED) {
        app_camera_cmd(cmd);
        return;
    }

    /* 录像按钮自锁：状态翻转后更新文字 */
    if(code == LV_EVENT_VALUE_CHANGED && btn == g_btn_rec && g_rec_lb != NULL) {
        lv_label_set_text(g_rec_lb, lv_obj_has_state(btn, LV_STATE_CHECKED) ? "录像中" : "录像");
    }
}

/*---------------------------------------------------------------- 构件 */

static lv_obj_t * side_btn(lv_obj_t * par, const char * txt, lv_coord_t y, int cmd)
{
    lv_obj_t * btn = lv_btn_create(par);
    lv_obj_remove_style_all(btn);
    lv_obj_set_pos(btn, 14, y);
    lv_obj_set_size(btn, 200, 44);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(C_NAV_OFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_user_data(btn, (void *)(intptr_t)cmd);
    lv_obj_add_event_cb(btn, cam_btn_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t * lb = lv_label_create(btn);
    lv_label_set_text(lb, txt);
    lv_obj_set_style_text_font(lb, &ui_font_cn24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lb, lv_color_hex(C_TEXT), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lb, lv_color_hex(C_TEXT_INV), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_center(lb);

    return btn;
}

/* 视频区四角的取景框角标 */
static void corner(lv_obj_t * par, lv_coord_t x, lv_coord_t y, lv_border_side_t side)
{
    lv_obj_t * c = lv_obj_create(par);
    lv_obj_remove_style_all(c);
    lv_obj_set_pos(c, x, y);
    lv_obj_set_size(c, 26, 26);
    lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(c, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(c, side, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(c, lv_color_hex(0x35E0A1), LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void anim_x_cb(void * obj, int32_t v)
{
    lv_obj_set_x(obj, (lv_coord_t)v);
}

static void anim_opa_cb(void * obj, int32_t v)
{
    lv_obj_set_style_bg_opa(obj, (lv_opa_t)v, LV_PART_MAIN | LV_STATE_DEFAULT);
}

/*---------------------------------------------------------------- 构建 */

void ui_Screen2_screen_init(void)
{
    lv_obj_t * vid, * side, * obj;
    lv_anim_t a;

    ui_Screen2 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen2, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen2, lv_color_hex(C_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen2, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_header_create(ui_Screen2, UI_SCR_CAMERA);

    /*---------------- 视频区 ----------------*/
    vid = lv_obj_create(ui_Screen2);
    lv_obj_remove_style_all(vid);
    lv_obj_set_pos(vid, VID_X, VID_Y);
    lv_obj_set_size(vid, VID_W, VID_H);
    lv_obj_clear_flag(vid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(vid, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(vid, lv_color_hex(0x0B1220), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(vid, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(vid, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(vid, lv_color_hex(0x2A3A55), LV_PART_MAIN | LV_STATE_DEFAULT);

    /* 模拟画面 */
    g_scene = lv_obj_create(vid);
    lv_obj_remove_style_all(g_scene);
    lv_obj_set_pos(g_scene, 0, 0);
    lv_obj_set_size(g_scene, VID_W, VID_H);
    lv_obj_clear_flag(g_scene, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(g_scene, LV_OBJ_FLAG_CLICKABLE);

    obj = lv_obj_create(g_scene);            /* 地面 */
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj, 20, 258);
    lv_obj_set_size(obj, VID_W - 40, 4);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x1E2C44), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    obj = lv_obj_create(g_scene);            /* 机架 */
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj, 60, 120);
    lv_obj_set_size(obj, 130, 138);
    lv_obj_set_style_radius(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x22334D), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    obj = lv_obj_create(g_scene);            /* 管道 */
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj, 210, 150);
    lv_obj_set_size(obj, 240, 16);
    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x22334D), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    obj = lv_obj_create(g_scene);            /* 传送带上移动的工件 */
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj, 40, 208);
    lv_obj_set_size(obj, 74, 46);
    lv_obj_set_style_radius(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x3E6FD6), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, 40, VID_W - 130);
    lv_anim_set_time(&a, 4500);
    lv_anim_set_playback_time(&a, 4500);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&a, anim_x_cb);
    lv_anim_start(&a);

    corner(vid, 12, 12, LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_TOP);
    corner(vid, VID_W - 38, 12, LV_BORDER_SIDE_RIGHT | LV_BORDER_SIDE_TOP);
    corner(vid, 12, VID_H - 38, LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_BOTTOM);
    corner(vid, VID_W - 38, VID_H - 38, LV_BORDER_SIDE_RIGHT | LV_BORDER_SIDE_BOTTOM);

    /* 叠加层 */
    g_src_lb = ui_text(vid, "CAM-01 · 电机区", &ui_font_cn16, 0xDCE6F2, 18, 14, 220, LV_TEXT_ALIGN_LEFT);
    ui_text(vid, "1920×1080 · 25 FPS", &ui_font_cn16, 0x8FA3BC,
            VID_W - 218, 14, 200, LV_TEXT_ALIGN_RIGHT);

    g_rec_dot = lv_obj_create(vid);
    lv_obj_remove_style_all(g_rec_dot);
    lv_obj_set_pos(g_rec_dot, 18, VID_H - 34);
    lv_obj_set_size(g_rec_dot, 12, 12);
    lv_obj_clear_flag(g_rec_dot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(g_rec_dot, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(g_rec_dot, lv_color_hex(C_ALARM), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(g_rec_dot, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_anim_init(&a);
    lv_anim_set_var(&a, g_rec_dot);
    lv_anim_set_values(&a, 255, 40);
    lv_anim_set_time(&a, 700);
    lv_anim_set_playback_time(&a, 700);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&a, anim_opa_cb);
    lv_anim_start(&a);

    ui_text(vid, "REC", &ui_font_cn16, 0xDCE6F2, 36, VID_H - 35, 60, LV_TEXT_ALIGN_LEFT);

    g_time_lb = ui_text(vid, "--:--:--", &ui_font_cn16, 0xDCE6F2,
                        VID_W - 218, VID_H - 35, 200, LV_TEXT_ALIGN_RIGHT);

    /* 无信号提示 */
    g_nosig = ui_text(vid, "无视频信号", &ui_font_cn24, 0x5B6B80, 0, VID_H / 2 - 16,
                      VID_W, LV_TEXT_ALIGN_CENTER);
    lv_obj_add_flag(g_nosig, LV_OBJ_FLAG_HIDDEN);

    /*---------------- 右侧信息栏 ----------------*/
    side = ui_card(ui_Screen2, SIDE_X, VID_Y, SIDE_W, VID_H);
    ui_text(side, "摄像头信息", &ui_font_cn24, C_TEXT, 14, 12, 200, LV_TEXT_ALIGN_LEFT);

    ui_text(side, "分辨率", &ui_font_cn16, C_TEXT_DIM, 14, 52, 100, LV_TEXT_ALIGN_LEFT);
    ui_text(side, "1920×1080", &ui_font_cn16, C_TEXT, 114, 52, 100, LV_TEXT_ALIGN_RIGHT);

    ui_text(side, "帧率", &ui_font_cn16, C_TEXT_DIM, 14, 80, 100, LV_TEXT_ALIGN_LEFT);
    ui_text(side, "25 FPS", &ui_font_cn16, C_TEXT, 114, 80, 100, LV_TEXT_ALIGN_RIGHT);

    ui_text(side, "码率", &ui_font_cn16, C_TEXT_DIM, 14, 108, 100, LV_TEXT_ALIGN_LEFT);
    ui_text(side, "4.2 Mbps", &ui_font_cn16, C_TEXT, 114, 108, 100, LV_TEXT_ALIGN_RIGHT);

    ui_text(side, "状态", &ui_font_cn16, C_TEXT_DIM, 14, 136, 100, LV_TEXT_ALIGN_LEFT);
    g_state_v = ui_text(side, "在线", &ui_font_cn16, C_OK, 114, 136, 100, LV_TEXT_ALIGN_RIGHT);

    g_btn_rec = side_btn(side, "录像", 170, 1);
    lv_obj_add_flag(g_btn_rec, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_style_bg_color(g_btn_rec, lv_color_hex(C_ALARM), LV_PART_MAIN | LV_STATE_CHECKED);
    g_rec_lb = lv_obj_get_child(g_btn_rec, 0);

    side_btn(side, "拍照", 222, 0);
    side_btn(side, "切换摄像头", 274, 2);

    ui_navbar_create(ui_Screen2, UI_SCR_CAMERA);
}

void ui_Screen2_screen_destroy(void)
{
    if(ui_Screen2) lv_obj_del(ui_Screen2);
    ui_Screen2  = NULL;
    g_scene     = NULL;
    g_nosig     = NULL;
    g_rec_dot   = NULL;
    g_time_lb   = NULL;
    g_state_v   = NULL;
    g_btn_rec   = NULL;
    g_rec_lb    = NULL;
    g_src_lb    = NULL;
}

/* 切换摄像头时更新左上角机位名 */
void ui_screen2_set_source(int idx)
{
    if(g_src_lb == NULL) return;
    lv_label_set_text(g_src_lb, idx == 0 ? "CAM-01 · 电机区" : "CAM-02 · 水泵区");
}

/*---------------------------------------------------------------- 刷新 */

void ui_screen2_update(const char * time_str, bool online)
{
    if(ui_Screen2 == NULL) return;

    if(g_time_lb != NULL && time_str != NULL) lv_label_set_text(g_time_lb, time_str);

    if(g_state_v != NULL) {
        lv_label_set_text(g_state_v, online ? "在线" : "离线");
        lv_obj_set_style_text_color(g_state_v, lv_color_hex(online ? C_OK : C_ALARM),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if(g_scene != NULL) {
        if(online) lv_obj_clear_flag(g_scene, LV_OBJ_FLAG_HIDDEN);
        else       lv_obj_add_flag(g_scene, LV_OBJ_FLAG_HIDDEN);
    }
    if(g_nosig != NULL) {
        if(online) lv_obj_add_flag(g_nosig, LV_OBJ_FLAG_HIDDEN);
        else       lv_obj_clear_flag(g_nosig, LV_OBJ_FLAG_HIDDEN);
    }
}
