// 工业用电设备监测终端 —— 主程序
//
// 采集（子线程） -> 判异 -> 界面刷新（LVGL 定时器）
// 监测对象：1 台电机 + 1 台水泵
//   电气参数：电压 / 电流 / 功率 / 振动强度（每台设备一套）
//   环境参数：温度 / 湿度 / 烟雾浓度（共用一套）
// 界面：主界面 / 摄像头 / 历史数据 / 阈值设置，屏幕最下方为四个界面的快捷入口

#include "lvgl/lvgl.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#include "demo1/ui.h"
#include "demo1/ui_common.h"
#include "demo1/screens/ui_Screen2.h"
#include "demo1/screens/ui_Screen3.h"
#include "demo1/screens/ui_Screen4.h"

/* 绘制缓冲开到整屏 (800x480)：一次画完，避免被切成多块反复重绘 */
#define DISP_BUF_SIZE (800 * 480)

/* UI 刷新周期 (ms)。数据源 1s 更新一次，500ms 足够 */
#define UI_UPDATE_PERIOD 500

/* 采集周期 (s)，同时也是历史曲线的采样间隔 */
#define SAMPLE_PERIOD 1

/* 阈值落盘位置 */
#define THR_FILE "threshold.cfg"

/*=================== 数据模型 ===================*/

/* 采集线程写、UI 线程读，用 g_lock 保护 */
static ui_dev_t g_motor;
static ui_dev_t g_pump;
static ui_env_t g_env;

static bool g_motor_run = true;
static bool g_pump_run  = true;

/* 阈值表，编号见 ui_Screen4.h */
static int32_t g_thr[THR_CNT];

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

/* 顶栏“数据上传云服务器”开关 */
static bool g_cloud_on = false;

/* 历史曲线的电气通道看电机还是水泵：0=电机 1=水泵 */
static int g_hist_src = 0;

/* 摄像头通道 */
static int g_cam_idx = 0;

/*=================== 历史曲线 ===================*/

/* 每个通道一份双缓冲环形数组：采集线程写 back，LVGL 读 front，写完原子换页，
 * 避免 LVGL 读到写了一半的数组。 */
typedef struct {
    lv_coord_t   buf[2][UI_HIST_POINTS];
    volatile int front;
} hist_buf_t;

static hist_buf_t g_hist_env[3];       /* 温度 / 湿度 / 烟雾 */
static hist_buf_t g_hist_elec[2][4];   /* [设备][电压 / 电流 / 功率 / 振动] */

static lv_chart_series_t * g_ser[UI_HIST_CNT];
static int                 g_bound[UI_HIST_CNT];   /* 该通道当前绑定到哪一页缓冲 */

/* 各通道满量程。曲线画的是“占量程百分比”，所以温湿度和几百伏的电压能同轴显示 */
static const int32_t g_fs[UI_HIST_CNT] = {
    120,    /* 温度 ℃      */
    100,    /* 湿度 %      */
    1000,   /* 烟雾 ppm    */
    500,    /* 电压 V      */
    50,     /* 电流 A      */
    3000,   /* 功率 W      */
    50,     /* 振动 mm/s   */
};

static void hist_push(hist_buf_t * h, int32_t v)
{
    int back = 1 - h->front;

    memcpy(h->buf[back], h->buf[h->front], sizeof(h->buf[0]));
    memmove(h->buf[back], h->buf[back] + 1, (UI_HIST_POINTS - 1) * sizeof(lv_coord_t));
    h->buf[back][UI_HIST_POINTS - 1] = (lv_coord_t)v;

    h->front = back;
}

static void hist_clear_all(void)
{
    int i, k;

    for(i = 0; i < 3; i++) memset(g_hist_env[i].buf, 0, sizeof(g_hist_env[i].buf));
    for(i = 0; i < 2; i++) {
        for(k = 0; k < 4; k++) memset(g_hist_elec[i][k].buf, 0, sizeof(g_hist_elec[i][k].buf));
    }
    for(i = 0; i < UI_HIST_CNT; i++) g_bound[i] = -1;   /* -1 强制重新绑定 */
}

/* 曲线只创建一次（幂等）。以前每 5ms add_series 一次导致曲线无限堆积，是卡顿根因 */
static void hist_create_series(void)
{
    int i;
    if(ui_HistChart == NULL) return;

    for(i = 0; i < UI_HIST_CNT; i++) {
        if(g_ser[i] != NULL) continue;
        g_ser[i] = lv_chart_add_series(ui_HistChart, lv_color_hex(ui_hist_color[i]),
                                       LV_CHART_AXIS_PRIMARY_Y);
    }
}

/* 按当前数据源把七个通道绑到曲线上 */
static void hist_bind(void)
{
    int i, k;

    if(ui_HistChart == NULL) return;

    /* 0~2 环境量 */
    for(i = 0; i < 3; i++) {
        if(g_ser[i] == NULL) continue;
        if(g_bound[i] != g_hist_env[i].front) {
            lv_chart_set_ext_y_array(ui_HistChart, g_ser[i],
                                     g_hist_env[i].buf[g_hist_env[i].front]);
            g_bound[i] = g_hist_env[i].front;
        }
    }

    /* 3~6 电气量：电压 / 电流 / 功率 / 振动，跟随“数据源”按钮 */
    for(k = 0; k < 4; k++) {
        hist_buf_t * h = &g_hist_elec[g_hist_src][k];
        i = 3 + k;
        if(g_ser[i] == NULL) continue;
        if(g_bound[i] != h->front) {
            lv_chart_set_ext_y_array(ui_HistChart, g_ser[i], h->buf[h->front]);
            g_bound[i] = h->front;
        }
    }
}

static void hist_sync(void)
{
    if(ui_HistChart == NULL) return;
    hist_bind();
    lv_chart_refresh(ui_HistChart);
}

static int32_t hist_norm(int ch, float v)
{
    int32_t pct;

    if(g_fs[ch] <= 0) return 0;
    pct = (int32_t)(v * 100.0f / (float)g_fs[ch]);
    if(pct < 0)   pct = 0;
    if(pct > 100) pct = 100;
    return pct;
}

/*=================== 采集 ===================*/

static float around(float base, float amp)
{
    return base + ((float)(rand() % 2001) - 1000.0f) / 1000.0f * amp;
}

static void * sample_task(void * arg)
{
    (void)arg;
    int tick = 0;

    while(1) {
        ui_dev_t motor, pump;
        ui_env_t env;

        /* 电机：380V 三相，电流随负载波动 */
        motor.volt  = around(380.0f, 8.0f);
        motor.curr  = around(12.0f, 3.0f);
        motor.power = motor.volt * motor.curr * 0.86f;
        motor.vib   = around(2.6f, 1.2f);

        /* 水泵：电流更小、振动更低 */
        pump.volt  = around(380.0f, 8.0f);
        pump.curr  = around(8.0f, 2.0f);
        pump.power = pump.volt * pump.curr * 0.84f;
        pump.vib   = around(1.8f, 0.9f);

        /* 环境 */
        env.temp  = around(32.0f, 5.0f);
        env.hum   = around(55.0f, 10.0f);
        env.smoke = around(90.0f, 60.0f);

        /* 偶发工况异常，用来验证报警链路。接真实传感器时删掉这段即可 */
        tick++;
        if(tick % 37 == 0) motor.vib   = around(12.0f, 2.0f);    /* 电机振动超限 */
        if(tick % 53 == 0) motor.volt  = around(300.0f, 10.0f);  /* 电机欠压     */
        if(tick % 61 == 0) env.temp    = around(78.0f, 5.0f);    /* 温度超限     */
        if(tick % 71 == 0) env.smoke   = around(520.0f, 60.0f);  /* 烟雾超限     */
        if(tick % 89 == 0) pump.curr   = around(26.0f, 3.0f);    /* 水泵过流     */

        if(env.smoke < 0)   env.smoke = 0;
        if(env.hum < 0)     env.hum   = 0;
        if(env.hum > 100)   env.hum   = 100;
        if(motor.vib < 0)   motor.vib = 0;
        if(pump.vib < 0)    pump.vib  = 0;

        pthread_mutex_lock(&g_lock);
        g_motor = motor;
        g_pump  = pump;
        g_env   = env;
        pthread_mutex_unlock(&g_lock);

        /* 历史曲线 */
        hist_push(&g_hist_env[0], hist_norm(UI_HIST_TEMP,  env.temp));
        hist_push(&g_hist_env[1], hist_norm(UI_HIST_HUM,   env.hum));
        hist_push(&g_hist_env[2], hist_norm(UI_HIST_SMOKE, env.smoke));

        hist_push(&g_hist_elec[0][0], hist_norm(UI_HIST_VOLT,  motor.volt));
        hist_push(&g_hist_elec[0][1], hist_norm(UI_HIST_CURR,  motor.curr));
        hist_push(&g_hist_elec[0][2], hist_norm(UI_HIST_POWER, motor.power));
        hist_push(&g_hist_elec[0][3], hist_norm(UI_HIST_VIB,   motor.vib));

        hist_push(&g_hist_elec[1][0], hist_norm(UI_HIST_VOLT,  pump.volt));
        hist_push(&g_hist_elec[1][1], hist_norm(UI_HIST_CURR,  pump.curr));
        hist_push(&g_hist_elec[1][2], hist_norm(UI_HIST_POWER, pump.power));
        hist_push(&g_hist_elec[1][3], hist_norm(UI_HIST_VIB,   pump.vib));

        sleep(SAMPLE_PERIOD);
    }

    return NULL;
}

/*=================== 阈值判定 ===================*/

static void eval_alarm(ui_snap_t * s)
{
    s->motor.volt_al  = (s->motor.volt > (float)g_thr[THR_VOLT_MAX]) ||
                        (s->motor.volt < (float)g_thr[THR_VOLT_MIN]);
    s->motor.curr_al  = (s->motor.curr  > (float)g_thr[THR_CURR_MAX]);
    s->motor.power_al = (s->motor.power > (float)g_thr[THR_POWER_MAX]);
    s->motor.vib_al   = (s->motor.vib   > (float)g_thr[THR_VIB_MAX]);

    s->pump.volt_al   = (s->pump.volt > (float)g_thr[THR_VOLT_MAX]) ||
                        (s->pump.volt < (float)g_thr[THR_VOLT_MIN]);
    s->pump.curr_al   = (s->pump.curr  > (float)g_thr[THR_CURR_MAX]);
    s->pump.power_al  = (s->pump.power > (float)g_thr[THR_POWER_MAX]);
    s->pump.vib_al    = (s->pump.vib   > (float)g_thr[THR_VIB_MAX]);

    s->env.temp_al    = (s->env.temp  > (float)g_thr[THR_TEMP_MAX]);
    s->env.hum_al     = (s->env.hum   > (float)g_thr[THR_HUM_MAX]);
    s->env.smoke_al   = (s->env.smoke > (float)g_thr[THR_SMOKE_MAX]);

    s->motor_alarm = s->motor.volt_al || s->motor.curr_al || s->motor.power_al || s->motor.vib_al;
    s->pump_alarm  = s->pump.volt_al  || s->pump.curr_al  || s->pump.power_al  || s->pump.vib_al;
    s->env_alarm   = s->env.temp_al   || s->env.hum_al    || s->env.smoke_al;
}

/*=================== 阈值持久化 ===================*/

static void thr_load(void)
{
    FILE * fp = fopen(THR_FILE, "r");
    int i;

    memcpy(g_thr, ui_thr_def, sizeof(g_thr));

    if(fp == NULL) return;
    for(i = 0; i < THR_CNT; i++) {
        long v;
        if(fscanf(fp, "%ld", &v) != 1) break;
        if(v < ui_thr_min[i]) v = ui_thr_min[i];
        if(v > ui_thr_max[i]) v = ui_thr_max[i];
        g_thr[i] = (int32_t)v;
    }
    fclose(fp);
}

static void thr_save(void)
{
    FILE * fp = fopen(THR_FILE, "w");
    int i;

    if(fp == NULL) {
        ui_screen4_hint("保存失败", false);
        return;
    }
    for(i = 0; i < THR_CNT; i++) fprintf(fp, "%ld\n", (long)g_thr[i]);
    fclose(fp);

    ui_screen4_hint("已保存", true);
}

/*=================== 云上传 ===================*/

/* 开关打开时把一帧数据往云上发。这里用打印代替，接 MQTT / HTTP 时替换掉即可 */
static void cloud_push(const ui_snap_t * s, uint32_t seq)
{
    if(!g_cloud_on) return;
    if(seq % 5 != 0) return;          /* 采集 1s 一次，这里 5s 发一条，别刷屏 */

    printf("[cloud] seq=%u | 电机 %.1fV %.1fA %.0fW 振动%.1f | 水泵 %.1fV %.1fA %.0fW 振动%.1f "
           "| 环境 %.1f℃ %.1f%% %.0fppm\n",
           (unsigned)seq,
           (double)s->motor.volt, (double)s->motor.curr, (double)s->motor.power, (double)s->motor.vib,
           (double)s->pump.volt,  (double)s->pump.curr,  (double)s->pump.power,  (double)s->pump.vib,
           (double)s->env.temp,   (double)s->env.hum,    (double)s->env.smoke);
    fflush(stdout);
}

/*=================== 界面回调 ===================*/

static lv_obj_t * g_scr[UI_SCR_CNT];
static int         g_cur_scr = UI_SCR_HOME;
static lv_obj_t  * g_cursor  = NULL;

void app_nav_to(int idx)
{
    if(idx < 0 || idx >= UI_SCR_CNT) return;
    if(g_scr[idx] == NULL) return;

    g_cur_scr = idx;
    lv_scr_load(g_scr[idx]);
    ui_nav_set_active(idx);

    /* 光标挂在旧界面上，切屏时跟着搬过去 */
    if(g_cursor != NULL) lv_obj_set_parent(g_cursor, g_scr[idx]);
}

void app_cloud_set(bool on)
{
    g_cloud_on = on;
    printf("[cloud] 数据上传%s\n", on ? "已开启" : "已关闭");
    ui_header_set_cloud(on);
}

void app_hist_channel(int idx, bool on)
{
    if(idx < 0 || idx >= UI_HIST_CNT) return;
    if(g_ser[idx] != NULL && ui_HistChart != NULL) {
        lv_chart_hide_series(ui_HistChart, g_ser[idx], !on);
    }
}

void app_hist_source(int dev)
{
    int i;
    if(dev != 0 && dev != 1) return;

    g_hist_src = dev;
    /* 电气通道换了数据源，强制重新绑定 */
    for(i = 3; i < UI_HIST_CNT; i++) g_bound[i] = -1;
    hist_sync();
}

void app_hist_clear(void)
{
    hist_clear_all();
    hist_sync();
}

void app_camera_cmd(int cmd)
{
    switch(cmd) {
    case 0:
        printf("[camera] 抓拍快照\n");
        break;
    case 1:
        printf("[camera] 录像状态切换\n");
        break;
    case 2:
        g_cam_idx = (g_cam_idx + 1) % 2;
        ui_screen2_set_source(g_cam_idx);
        printf("[camera] 切换到 CAM-0%d\n", g_cam_idx + 1);
        break;
    default:
        break;
    }
}

void app_thr_set(int id, int32_t val)
{
    if(id < 0 || id >= THR_CNT) return;
    if(val < ui_thr_min[id]) val = ui_thr_min[id];
    if(val > ui_thr_max[id]) val = ui_thr_max[id];

    g_thr[id] = val;
}

void app_thr_save(void)
{
    thr_save();
}

void app_thr_reset(void)
{
    memcpy(g_thr, ui_thr_def, sizeof(g_thr));
    ui_screen4_sync(g_thr, THR_CNT);
    ui_screen4_hint("已恢复默认", true);
}

/*=================== 界面刷新 ===================*/

static uint32_t g_seq = 0;

static void ui_update_timer_cb(lv_timer_t * t)
{
    ui_snap_t snap;
    char tbuf[32];
    time_t now;
    struct tm * tm_now;

    LV_UNUSED(t);

    now = time(NULL);
    tm_now = localtime(&now);
    if(tm_now == NULL) return;

    /* 顶栏时间 */
    strftime(tbuf, sizeof(tbuf), "%m-%d %H:%M", tm_now);
    ui_header_set_time(tbuf);

    /* 取实时值并判异 */
    memset(&snap, 0, sizeof(snap));
    pthread_mutex_lock(&g_lock);
    snap.motor = g_motor;
    snap.pump  = g_pump;
    snap.env   = g_env;
    snap.motor_run = g_motor_run;
    snap.pump_run  = g_pump_run;
    pthread_mutex_unlock(&g_lock);

    eval_alarm(&snap);

    g_seq++;
    cloud_push(&snap, g_seq);

    /* 只刷当前界面，看不见的界面不做无用重绘 */
    switch(g_cur_scr) {
    case UI_SCR_HOME:
        ui_screen1_update(&snap);
        break;

    case UI_SCR_CAMERA:
        strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", tm_now);
        ui_screen2_update(tbuf, true);
        break;

    case UI_SCR_HISTORY:
        ui_screen3_update(&snap);
        hist_sync();
        break;

    case UI_SCR_THRESHOLD:
    default:
        break;
    }
}

/*=================== main ===================*/

int main(void)
{
    pthread_t pid;

    lv_init();
    fbdev_init();

    /* A buffer for LittlevGL to draw the screen's content (整屏大小) */
    static lv_color_t buf[DISP_BUF_SIZE];
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, DISP_BUF_SIZE);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf  = &disp_buf;
    disp_drv.flush_cb  = fbdev_flush;
    disp_drv.hor_res   = 800;
    disp_drv.ver_res   = 480;
    lv_disp_drv_register(&disp_drv);

    evdev_init();
    static lv_indev_drv_t indev_drv_1;
    lv_indev_drv_init(&indev_drv_1);
    indev_drv_1.type    = LV_INDEV_TYPE_POINTER;
    indev_drv_1.read_cb = evdev_read;
    lv_indev_t * mouse_indev = lv_indev_drv_register(&indev_drv_1);

    thr_load();
    hist_clear_all();

    ui_init();

    g_scr[UI_SCR_HOME]      = ui_Screen1;
    g_scr[UI_SCR_CAMERA]    = ui_Screen2;
    g_scr[UI_SCR_HISTORY]   = ui_Screen3;
    g_scr[UI_SCR_THRESHOLD] = ui_Screen4;

    /* 鼠标光标：纯触屏场景可以整段注释掉。挂在 lv_scr_act() 上，切屏时由 app_nav_to 搬走 */
    LV_IMG_DECLARE(mouse_cursor_icon)
    g_cursor = lv_img_create(lv_scr_act());
    lv_img_set_src(g_cursor, &mouse_cursor_icon);
    lv_indev_set_cursor(mouse_indev, g_cursor);

    hist_create_series();
    hist_bind();
    ui_screen4_sync(g_thr, THR_CNT);
    ui_header_set_cloud(g_cloud_on);
    app_nav_to(UI_SCR_HOME);

    /* 先跑一次，把各界面填上有数据的初值 */
    ui_update_timer_cb(NULL);
    lv_timer_create(ui_update_timer_cb, UI_UPDATE_PERIOD, NULL);

    pthread_create(&pid, NULL, sample_task, NULL);

    while(1) {
        lv_timer_handler();
        usleep(5000);
    }

    return 0;
}

/*Set in lv_conf.h as `LV_TICK_CUSTOM_SYS_TIME_EXPR`*/
uint32_t custom_tick_get(void)
{
    static uint64_t start_ms = 0;
    struct timeval tv_start;
    struct timeval tv_now;
    uint64_t now_ms;

    if(start_ms == 0) {
        gettimeofday(&tv_start, NULL);
        start_ms = (uint64_t)(tv_start.tv_sec * 1000000 + tv_start.tv_usec) / 1000;
    }

    gettimeofday(&tv_now, NULL);
    now_ms = (uint64_t)(tv_now.tv_sec * 1000000 + tv_now.tv_usec) / 1000;

    return (uint32_t)(now_ms - start_ms);
}
