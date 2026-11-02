// 工业用电设备监测终端 —— 主程序
//
// 线程模型：
//   get_data   : 采集（这里是模拟数据，接真实传感器时只改这个线程）
//   show_data  : 把一帧数据投给 UI（只写快照，不碰 lv_*）
//   save_data  : 落库
//   send_data  : MQTT 上传（受界面“数据上传云服务器”开关控制）
//   主线程      : LVGL 初始化 + lv_timer_handler 主循环（所有 lv_* 只在这里调用）
//
// LVGL 只允许在主线程操作，所以界面刷新走 lv_timer（见 ui_app.c），
// 采集线程和界面之间用快照 + 互斥锁解耦。

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#include "lvgl/lvgl.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"

#include "mylink.h"
#include "mailbox.h"
#include "qlink.h"
#include "mysqlite3.h"
#include "mymqtt.h"
#include "ui_app.h"
#include "mq2.h"
#include "spt660_read.h"

/* 绘制缓冲开到整屏 (800x480)：一次画完，避免被切成多块反复重绘 */
#define DISP_BUF_SIZE (UI_HOR_RES * UI_VER_RES)

/* 采集周期 (s)，同时也是历史曲线的采样间隔 */
#define SAMPLE_PERIOD 1

/*=================== 采集 ===================*/

static float around(float base, float amp)
{
    return base + ((float)(rand() % 2001) - 1000.0f) / 1000.0f * amp;
}

int temp_hun_get(float *temp, float *humity)
{
    int fd = 0;
    ssize_t nsize = 0;
    unsigned char data[4] = {0};

    // 1. 打开dht11设备节点  /dev/misc_dht11
    fd = open("/dev/misc_dht11", O_RDWR);
    if (fd < 0)
    {
        perror("[APP]:fail to open misc_dht11!");
        return -1;
    }

    // 2. 读取温湿度数据
    nsize = read(fd, data, 4);
    if (nsize != 4)
    {
        perror("[APP]:fail to read dht11!");
    }

    *humity = data[0] + (data[1] / 100.0);
    *temp = data[2] + (data[3] / 100.0);

    return 0;
}

void *get_data_th(void *arg)
{
    struct msgdata mydata;
    int cnt = 0;
    int tick = 0;
    time_t sec;
    struct tm *ptm = NULL;

    (void)arg;

    while (1)
    {
        ui_dev_t motor, pump;
        ui_env_t env;
        int ret_smoke = 0;
        float R0 = 0;
        float Rs = 0;
        float ppmVal = 0;
        struct adc_dev imx6ulladc;

        /* 电机：380V 三相，电流随负载波动 */
        spt660_data_get(&motor.volt, &motor.curr, &motor.power);
        //motor.volt = 
        //motor.curr = 
        //motor.power = 
        motor.vib = 1;

        /* 水泵：电流更小、振动更低 */
        pump.volt = 380;
        pump.curr = 10;
        pump.power = 2000;
        pump.vib = 4;

        /* 环境 */
        // env.temp  = around(32.0f, 5.0f);
        // env.hum   = around(55.0f, 10.0f);
        temp_hun_get(&env.temp, &env.hum);
        ret_smoke = adc_read(&imx6ulladc);
        // 数学曲线拟合公式 实际电压转换气体浓度
        R0 = 6.64;
        Rs = (5 - imx6ulladc.act) / imx6ulladc.act * 0.5;
        // env.smoke = pow(11.5428 * R0 / Rs, 0.6549f) * 100;
        env.smoke = 100;

        /* 偶发工况异常，用来验证报警链路。接真实传感器时删掉这段即可 */
        /*
        tick++;
        if (tick % 37 == 0) motor.vib  = around(12.0f, 2.0f);    // 电机振动超限
        if (tick % 53 == 0) motor.volt = around(300.0f, 10.0f);  // 电机欠压
        if (tick % 61 == 0) env.temp   = around(78.0f, 5.0f);    // 温度超限
        if (tick % 71 == 0) env.smoke  = around(520.0f, 60.0f);  // 烟雾超限
        if (tick % 89 == 0) pump.curr  = around(26.0f, 3.0f);    // 水泵过流

        if (env.smoke < 0) env.smoke = 0;
        if (env.hum < 0)   env.hum   = 0;
        if (env.hum > 100) env.hum   = 100;
        if (motor.vib < 0) motor.vib = 0;
        if (pump.vib < 0)  pump.vib  = 0;
        */
        time(&sec);
        ptm = localtime(&sec);

        /* --- 电机这一帧：id=1 --- */
        memset(&mydata, 0, sizeof(mydata));
        mydata.id = 1;
        mydata.cnt = cnt;
        mydata.v = motor.volt;
        mydata.a = motor.curr;
        mydata.power = motor.power;
        mydata.vib = motor.vib;
        mydata.temp = env.temp;
        mydata.hum = env.hum;
        mydata.smoke = env.smoke;
        if (ptm != NULL)
            mydata.tim = *ptm;

        send_mail_msg("get_data", mydata, "show_data");
        send_mail_msg("get_data", mydata, "save_data");
        send_mail_msg("get_data", mydata, "send_data");

        /* --- 水泵这一帧：id=2 --- */
        mydata.id = 2;
        mydata.v = pump.volt;
        mydata.a = pump.curr;
        mydata.power = pump.power;
        mydata.vib = pump.vib;

        send_mail_msg("get_data", mydata, "show_data");
        send_mail_msg("get_data", mydata, "save_data");
        send_mail_msg("get_data", mydata, "send_data");

        cnt++;
        sleep(SAMPLE_PERIOD);
    }

    return NULL;
}

/*=================== 显示（投递给 UI） ===================*/

void *show_data_th(void *arg)
{
    struct msgdata mydata;
    char send_name[32] = {0};
    int ret = 0;

    (void)arg;

    while (1)
    {
        ret = recv_mail_msg(&mydata, send_name);
        if (0 != ret)
        {
            continue;
        }

        ui_dev_t dev;
        ui_env_t env;

        dev.volt = mydata.v;
        dev.curr = mydata.a;
        dev.power = mydata.power;
        dev.vib = mydata.vib;
        dev.volt_al = dev.curr_al = dev.power_al = dev.vib_al = false;

        env.temp = mydata.temp;
        env.hum = mydata.hum;
        env.smoke = mydata.smoke;
        env.temp_al = env.hum_al = env.smoke_al = false;

        /* 只投递数据，界面刷新交给主线程的 lv_timer */
        ui_app_feed(mydata.id - 1, &dev, &env);

        printf("%s->show: id=%d cnt=%d %.1fV %.1fA %.0fW vib%.1f | %.1fC %.1f%% %.0fppm\n", send_name, mydata.id,
               mydata.cnt, mydata.v, mydata.a, mydata.power, mydata.vib, mydata.temp, mydata.hum, mydata.smoke);
    }

    return NULL;
}

/*=================== 存储 ===================*/

void *save_data_th(void *arg)
{
    struct msgdata mydata;
    char send_name[32] = {0};
    int ret = 0;

    (void)arg;

    sqlite3_init();
    while (1)
    {
        ret = recv_mail_msg(&mydata, send_name);
        if (0 != ret)
        {
            continue;
        }
        /* TODO: 换成 insert into 实时数据表，按 id/cnt 存 v/a/power/vib/temp/hum/smoke */
        /*printf("%s->save: id=%d cnt=%d temp=%.1f hum=%.1f\n",
               send_name, mydata.id, mydata.cnt, mydata.temp, mydata.hum);*/
    }

    return NULL;
}

/*=================== 上传 ===================*/

/* mymqtt.h 里加两行声明；不想动头文件就保留这两行 extern */
extern int mqtt_pub_json(const char *json);
extern int mqtt_next_id(void);

/* 只上传电机帧（id=1）的 7 个量，key 和云端已有属性一一对应：
 *   v / a / w / vib  —— 电压、电流、功率、振动
 *   temp / hum / ppm —— 温度、湿度、烟雾
 * 水泵帧（id=2）不上云：物模型里没有它的属性坑位，
 * 而且它和电机共用同名 key，硬发会把电机刚写进去的值覆盖掉。
 * 水泵数据在本地 LCD 上照常显示，只是不往云端发。 */
static int pub_frame(const struct msgdata *d)
{
    static int up_cnt = 0;
    char json[512];
    int n;

    n = snprintf(json, sizeof(json),
                 "{\"id\":\"%d\",\"version\":\"1.0\",\"params\":{"
                 "\"v\":{\"value\":%.1f},"
                 "\"a\":{\"value\":%.2f},"
                 "\"w\":{\"value\":%.0f},"
                 "\"vib\":{\"value\":%.2f},"
                 "\"temp\":{\"value\":%.1f},"
                 "\"hum\":{\"value\":%.1f},"
                 "\"ppm\":{\"value\":%.0f}}}",
                 mqtt_next_id(), d->v, d->a, d->power, d->vib, d->temp, d->hum, d->smoke);

    if (n <= 0 || n >= (int)sizeof(json))
    {
        return -1;
    }

    /* 调试期打开，方便和云端逐条对照；稳定后注释掉即可 */
    printf("[send#%d] %s\n", ++up_cnt, json);

    return mqtt_pub_json(json);
}

void *send_data_th(void *arg)
{
    struct msgdata mydata;
    char send_name[32] = {0};
    int ret = 0;

    (void)arg;

    if (0 != mqtt_init())
    {
        printf("[send] mqtt_init failed\n");
        return NULL;
    }

    // 关键：init 只 create，必须显式 connect
    while (0 != mqtt_connect())
    {
        printf("[send] connect failed, retry in 5s\n");
        sleep(5);
    }

    while (1)
    {
        ret = recv_mail_msg(&mydata, send_name);
        if (0 != ret)
        {
            continue;
        }

        /* 界面顶栏的“数据上传云服务器”开关关掉时就不发 */
        if (!ui_app_cloud_on())
        {
            continue;
        }

        /* 水泵帧不上云，直接跳过 */
        if (2 == mydata.id)
        {
            continue;
        }

        /* 一帧一个报文一次发完。原来是 8 次 mqtt_send、每次末尾 sleep(1)，
         * 一帧要 8 秒，而采集每秒 1 帧，队列只会无限积压，
         * 云端看到的一直是几分钟前的历史值。 */
        pub_frame(&mydata);
    }

    mqtt_deinit();
    return NULL;
}

/*
void *send_data_th(void *arg)
{
    struct msgdata mydata;
    char send_name[32] = {0};
    int ret = 0;

    (void)arg;

    mqtt_init();

    if (0 != mqtt_init())
    {
        printf("[send] mqtt_init failed\n");
        return NULL;
    }

    // 关键：init 只 create，必须显式 connect
    while (0 != mqtt_connect())
    {
        printf("[send] connect failed, retry in 5s\n");
        sleep(5);
    }


    while (1)
    {
        ret = recv_mail_msg(&mydata, send_name);
        if (0 != ret)
        {
            continue;
        }

        // 界面顶栏的“数据上传云服务器”开关关掉时就不发
        if (!ui_app_cloud_on())
        {
            continue;
        }

        // printf("%s->send: id=%d cnt=%d\n", send_name, mydata.id, mydata.cnt);
        if(mydata.id == 1)
        {
        mqtt_send("id", mydata.id);
        mqtt_send("v", mydata.v);
        mqtt_send("a", mydata.a);
        mqtt_send("w", mydata.power);
        mqtt_send("vib", mydata.vib);
        mqtt_send("temp", mydata.temp);
        mqtt_send("hum", mydata.hum);
        mqtt_send("ppm", mydata.smoke);
        }
    }

    mqtt_deinit();
    return NULL;
}
*/

/*=================== LVGL 显示/输入初始化 ===================*/

static void display_init(void)
{
    lv_init();
    fbdev_init();

    /* 一整屏的绘制缓冲 */
    static lv_color_t buf[DISP_BUF_SIZE];
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, DISP_BUF_SIZE);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &disp_buf;
    disp_drv.flush_cb = fbdev_flush;
    disp_drv.hor_res = UI_HOR_RES;
    disp_drv.ver_res = UI_VER_RES;
    lv_disp_drv_register(&disp_drv);

    /* 触摸 / 鼠标：设备节点在 lv_drv_conf.h 的 EVDEV_NAME 里改 */
    evdev_init();
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = evdev_read;
    lv_indev_drv_register(&indev_drv);
}

/*=================== main ===================*/

int main(void)
{
    Link_t *pmail = NULL;

    /* 1. 显示与输入（lv_conf.h 里的 tick 用下面的 custom_tick_get） */
    display_init();

    /* 2. 界面：载入阈值、建屏、建曲线、注册刷新定时器 */
    ui_app_init();

    /* 3. 消息邮箱 + 四个线程 */
    pmail = mailbox_create_system();
    if (NULL == pmail)
    {
        perror("mailbox_create_system error");
        return -1;
    }

    /* 先注册消费者，最后注册采集线程：
     * register_thread_mailbox 里 pthread_create 是立即生效的，
     * 采集线程先起来的话第一批消息会 “Not recv name” 丢掉 */
    register_thread_mailbox(pmail, "show_data", show_data_th);
    register_thread_mailbox(pmail, "save_data", save_data_th);
    register_thread_mailbox(pmail, "send_data", send_data_th);
    register_thread_mailbox(pmail, "get_data", get_data_th);

    /* 4. LVGL 主循环。
     *    注意：这里不能像原来那样 destroy_mailbox(pmail) —— 它是 pthread_join，
     *    而四个线程都是 while(1)，主线程会永远阻塞在里面，界面就再也刷不出来了。 */
    while (1)
    {
        lv_timer_handler();
        usleep(5000);
    }

    destroy_mailbox(pmail); /* 走不到，保留以便以后加退出逻辑 */
    return 0;
}

/* lv_conf.h：LV_TICK_CUSTOM_SYS_TIME_EXPR (custom_tick_get()) —— LVGL 的毫秒时基 */
uint32_t custom_tick_get(void)
{
    static uint64_t start_ms = 0;
    struct timeval tv_start;
    struct timeval tv_now;
    uint64_t now_ms;

    if (start_ms == 0)
    {
        gettimeofday(&tv_start, NULL);
        start_ms = (uint64_t)(tv_start.tv_sec * 1000000 + tv_start.tv_usec) / 1000;
    }

    gettimeofday(&tv_now, NULL);
    now_ms = (uint64_t)(tv_now.tv_sec * 1000000 + tv_now.tv_usec) / 1000;

    return (uint32_t)(now_ms - start_ms);
}
