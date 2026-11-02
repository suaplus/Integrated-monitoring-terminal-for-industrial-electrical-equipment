#ifndef __UI_APP_H__
#define __UI_APP_H__

// UI 与业务之间的桥。
//
// main.c 只做三件事：初始化 LVGL 显示/输入、注册 mailbox 线程、跑 lv_timer_handler 主循环；
// 界面初始化、数据快照、历史曲线、阈值持久化和 app_* 回调都放在 ui_app.c 里。
//
// 为什么要拆：LVGL 不是线程安全的，lv_* 只允许在主线程调用。
// 采集线程把数据投递给 ui_app_feed()，它只写受互斥锁保护的快照 + 曲线环形缓冲，
// 真正的重绘由主线程里的 lv_timer 完成。

#include <stdbool.h>

#include "demo1/ui.h"       /* ui_dev_t / ui_env_t / ui_snap_t / UI_SCR_* */

/* lv_init() 并且显示、输入驱动注册好之后调用一次：
 * 载入阈值 -> 建界面 -> 建曲线 -> 绑数据源 -> 建刷新定时器 */
void ui_app_init(void);

/* 采集线程调用。dev：0=电机 1=水泵；e 为共用的环境量。
 * 只写内存，不碰任何 lv_* 接口，因此在任意线程里调用都安全。 */
void ui_app_feed(int dev, const ui_dev_t *d, const ui_env_t *e);

/* 顶栏“数据上传云服务器”开关状态，上传线程据此决定要不要发 MQTT */
bool ui_app_cloud_on(void);

#endif
