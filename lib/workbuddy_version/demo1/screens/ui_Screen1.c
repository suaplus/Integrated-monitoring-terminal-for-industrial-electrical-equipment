// 主界面（第一屏）
//
// 上半：电机 / 水泵两张设备卡 —— 运行状态 + 电压 / 电流 / 功率 / 振动强度
// 下半：环境参数 —— 温度 / 湿度 / 烟雾浓度
// 顶栏右侧：数据上传云服务器开关

#include "../ui.h"
#include "../ui_common.h"

#include <stdio.h>
#include <string.h>

/*---------------------------------------------------------------- 布局 */
#define CARD_W   382
#define CARD_H   188
#define CARD_Y   68
#define CARD1_X  12
#define CARD2_X  406

#define ENV_Y    268
#define ENV_H    142

#define CELL_W   175
#define CELL_H   62

/* 设备卡运行时对象 */
typedef struct {
    lv_obj_t * card;
    lv_obj_t * pill;         /* 状态胶囊     */
    lv_obj_t * pill_txt;
    ui_cell_t  cell[4];      /* 电压/电流/功率/振动 */
} dev_card_t;

lv_obj_t * ui_Screen1 = NULL;

static dev_card_t g_motor;
static dev_card_t g_pump;
static ui_cell_t  g_env[3];
static lv_obj_t * g_env_state = NULL;

/*---------------------------------------------------------------- 构件 */

/* 设备卡：左侧色条 + 名称 + 状态胶囊 + 2x2 数值格 */
static void dev_card_build(dev_card_t * d, lv_obj_t * par, lv_coord_t x,
                           const char * name, const char * tag, uint32_t accent)
{
    static const char * cell_title[4] = {"电压", "电流", "功率", "振动强度"};
    int i;

    d->card = ui_card(par, x, CARD_Y, CARD_W, CARD_H);

    /* 左侧主题色条 */
    lv_obj_t * bar = lv_obj_create(d->card);
    lv_obj_remove_style_all(bar);
    lv_obj_set_pos(bar, 0, 0);
    lv_obj_set_size(bar, 6, CARD_H);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(bar, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(bar, lv_color_hex(accent), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_text(d->card, name, &ui_font_cn24, C_TEXT, 18, 12, 56, LV_TEXT_ALIGN_LEFT);
    ui_text(d->card, tag, &ui_font_cn16, C_TEXT_DIM, 76, 17, 120, LV_TEXT_ALIGN_LEFT);

    /* 状态胶囊 */
    d->pill = lv_obj_create(d->card);
    lv_obj_remove_style_all(d->pill);
    lv_obj_set_pos(d->pill, CARD_W - 112, 12);
    lv_obj_set_size(d->pill, 100, 30);
    lv_obj_clear_flag(d->pill, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(d->pill, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(d->pill, lv_color_hex(C_OK), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(d->pill, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    d->pill_txt = lv_label_create(d->pill);
    lv_label_set_text(d->pill_txt, "正常运行");
    lv_obj_set_style_text_font(d->pill_txt, &ui_font_cn16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(d->pill_txt, lv_color_hex(C_TEXT_INV), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(d->pill_txt);

    /* 2x2 参数格 */
    for(i = 0; i < 4; i++) {
        lv_coord_t cx = (i % 2) ? 199 : 12;
        lv_coord_t cy = (i / 2) ? 120 : 50;
        ui_cell_init(&d->cell[i], d->card, cx, cy, CELL_W, CELL_H, cell_title[i], true);
    }
}

/* 状态胶囊：停机 > 报警 > 正常 */
static void dev_card_set_state(dev_card_t * d, bool run, bool alarm)
{
    uint32_t color = C_OK;
    const char * txt = "正常运行";

    if(!run) {
        color = C_IDLE;
        txt   = "已停机";
    }
    else if(alarm) {
        color = C_ALARM;
        txt   = "异常报警";
    }

    lv_obj_set_style_bg_color(d->pill, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(d->pill_txt, txt);
}

static void dev_card_set_value(dev_card_t * d, const ui_dev_t * v)
{
    char buf[24];
    int i;

    struct {
        float val;
        bool  al;
        const char * fmt;
    } item[4] = {
        {v->volt,  v->volt_al,  "%.1f V"},
        {v->curr,  v->curr_al,  "%.1f A"},
        {v->power, v->power_al, "%.0f W"},
        {v->vib,   v->vib_al,   "%.1f mm/s"},
    };

    for(i = 0; i < 4; i++) {
        snprintf(buf, sizeof(buf), item[i].fmt, (double)item[i].val);
        lv_label_set_text(d->cell[i].value, buf);
        lv_obj_set_style_text_color(d->cell[i].value,
                                    lv_color_hex(item[i].al ? C_ALARM : C_TEXT),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
        ui_cell_set_dot(&d->cell[i], item[i].al ? C_ALARM : C_OK);
    }
}

/*---------------------------------------------------------------- 构建 */

void ui_Screen1_screen_init(void)
{
    static const char * env_title[3] = {"温度", "湿度", "烟雾浓度"};
    int i;

    ui_Screen1 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen1, lv_color_hex(C_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen1, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_header_create(ui_Screen1, UI_SCR_HOME);

    /* 两张设备卡 */
    dev_card_build(&g_motor, ui_Screen1, CARD1_X, "电机", "MOTOR-01", C_MOTOR);
    dev_card_build(&g_pump,  ui_Screen1, CARD2_X, "水泵", "PUMP-01",  C_PUMP);

    /* 环境参数面板 */
    lv_obj_t * env = ui_card(ui_Screen1, UI_BODY_X, ENV_Y, UI_BODY_W, ENV_H);
    ui_text(env, "环境参数", &ui_font_cn24, C_TEXT, 16, 12, 120, LV_TEXT_ALIGN_LEFT);
    g_env_state = ui_text(env, "全部正常", &ui_font_cn16, C_OK, 620, 18, 140, LV_TEXT_ALIGN_RIGHT);

    for(i = 0; i < 3; i++) {
        ui_cell_init(&g_env[i], env, 16 + i * 252, 46, 240, 80, env_title[i], true);
    }

    ui_navbar_create(ui_Screen1, UI_SCR_HOME);
}

void ui_Screen1_screen_destroy(void)
{
    if(ui_Screen1) lv_obj_del(ui_Screen1);
    ui_Screen1   = NULL;
    g_env_state  = NULL;
    memset(&g_motor, 0, sizeof(g_motor));
    memset(&g_pump, 0, sizeof(g_pump));
    memset(g_env, 0, sizeof(g_env));
}

/*---------------------------------------------------------------- 刷新 */

void ui_screen1_update(const ui_snap_t * s)
{
    char buf[24];
    int i;

    if(ui_Screen1 == NULL || s == NULL) return;

    dev_card_set_state(&g_motor, s->motor_run, s->motor_alarm);
    dev_card_set_state(&g_pump,  s->pump_run,  s->pump_alarm);
    dev_card_set_value(&g_motor, &s->motor);
    dev_card_set_value(&g_pump,  &s->pump);

    snprintf(buf, sizeof(buf), "%.1f ℃", (double)s->env.temp);
    lv_label_set_text(g_env[0].value, buf);
    snprintf(buf, sizeof(buf), "%.1f %%", (double)s->env.hum);
    lv_label_set_text(g_env[1].value, buf);
    snprintf(buf, sizeof(buf), "%.0f ppm", (double)s->env.smoke);
    lv_label_set_text(g_env[2].value, buf);

    for(i = 0; i < 3; i++) {
        bool al = (i == 0) ? s->env.temp_al : ((i == 1) ? s->env.hum_al : s->env.smoke_al);
        lv_obj_set_style_text_color(g_env[i].value, lv_color_hex(al ? C_ALARM : C_TEXT),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
        ui_cell_set_dot(&g_env[i], al ? C_ALARM : C_OK);
    }

    lv_label_set_text(g_env_state, s->env_alarm ? "环境异常" : "全部正常");
    lv_obj_set_style_text_color(g_env_state, lv_color_hex(s->env_alarm ? C_ALARM : C_OK),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
}
