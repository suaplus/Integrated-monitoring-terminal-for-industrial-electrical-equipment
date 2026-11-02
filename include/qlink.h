#ifndef __QLINK_H__
#define __QLINK_H__

#include <time.h>

struct msgdata
{
      int id;        //设备id：1=电机 2=水泵
      int cnt;       //数据编号 
      float temp;    //环境温度 ℃
      float hum;     //环境湿度 %
      float  v;      //电压 V
      float a;       //电流 A
      float power;   //功率 W（UI 需要，采集侧算好再发）
      float vib;     //振动强度 mm/s
      float smoke;   //烟雾浓度 ppm
      //......
      struct tm tim;   //时间戳
};

struct msg
{
      char send_name[32];
      struct msgdata data;
      char recv_name[32];
};

typedef struct msg Data_t;

typedef struct qnode
{
    Data_t data;
    struct qnode *pnext;
}Qnode_t;

typedef struct qlink
{
    Qnode_t *phead;
    Qnode_t *ptail;
    int clen;
}Qlink_t;

extern Qlink_t *creat_queue();
extern int insert_qnode(Qlink_t *pqlink, Data_t data);
extern int show_qlink(Qlink_t *pqlink);
extern int goout_qlink(Qlink_t *pqlink, Data_t *pdata);
extern int get_head_qlink(Qlink_t *pqlink, Data_t *pdata);
extern int destroy_qlink(Qlink_t *pqlink);

#endif