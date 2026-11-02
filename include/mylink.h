#ifndef __MYLINK_H__
#define __MYLINK_H__

#include <pthread.h>
#include <semaphore.h>
#include "qlink.h"

typedef struct thread_task
{
    char name[32];          //线程任务名称
    pthread_t tid;          //线程id号
    void *(*pfun)(void *);  //线程要执行任务
    Qlink_t *pque;          //用来通信的队列
    pthread_mutex_t mutex;   //队列互斥锁
    sem_t sem;               //同步队列读写的信号量
} LData_t;

typedef struct node_t
{
    LData_t data;
    struct node_t *pnext;
} Node_t;

typedef struct link_t
{
    Node_t *phead;
    int clen;
} Link_t;

extern Link_t *creat_link();
extern int is_empty_link(Link_t *plink);
extern int insert_head_link(Link_t *plink, LData_t data);
extern void show_link(Link_t *plink);
extern int insert_end_link(Link_t *plink, LData_t data);
extern Node_t *creat_node(LData_t data);
extern int del_head_link(Link_t *plink);
extern int del_end_link(Link_t *plink);
extern void destroy_link(Link_t *plink);

#endif