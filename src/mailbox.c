#include "mailbox.h"
#include "mylink.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

Link_t *pmail = NULL;

Link_t *mailbox_create_system()
{
    pmail = creat_link();
    if (NULL == pmail)
    {
        perror("mailbox_create_system creat_link error");
        return NULL;
    }

    return pmail;
}

int register_thread_mailbox(Link_t *pmail, char *name, void *(*pfun)(void *))
{
    Node_t *pinsert = malloc(sizeof(Node_t));
    if (NULL == pinsert)
    {
        printf("malloc error\n");
        return -1;
    }

    strcpy(pinsert->data.name, name);
    pinsert->data.pfun = pfun;
    pinsert->data.pque = creat_queue();
    if (NULL == pinsert->data.pque)
    {
        printf("create_link_queue error\n");
        return -1;
    }

    pthread_mutex_init(&pinsert->data.mutex, NULL);
    sem_init(&pinsert->data.sem, 0, 0);

    pthread_create(&pinsert->data.tid, NULL, pinsert->data.pfun, NULL);

    pinsert->pnext = NULL;

    pinsert->pnext = pmail->phead;
    pmail->phead = pinsert;

    pmail->clen++;

    return 0;
}

void destroy_mailbox(Link_t *pmail)
{
    Node_t *ptmp = pmail->phead;
    while (ptmp != NULL)
    {
        pthread_join(ptmp->data.tid, NULL);
        destroy_qlink(ptmp->data.pque);
        ptmp = ptmp->pnext;
    }
    destroy_link(pmail);
}

int send_mail_msg(char *send_name, struct msgdata mydata, char *recv_name)
{
    Node_t *ptmp = pmail->phead;

    while (ptmp != NULL)
    {
        if (0 == strcmp(ptmp->data.name, recv_name))
        {
            break;
        }
        ptmp = ptmp->pnext;
    }
    if (NULL == ptmp)
    {
        printf("Not recv name\n");
        return -1;
    }
    struct msg qdata;
    qdata.data = mydata;
    strcpy(qdata.send_name, send_name);
    strcpy(qdata.recv_name, recv_name);

    pthread_mutex_lock(&ptmp->data.mutex);
    insert_qnode(ptmp->data.pque, qdata);
    sem_post(&ptmp->data.sem);
    pthread_mutex_unlock(&ptmp->data.mutex);

    return 0;
}

int recv_mail_msg(struct msgdata *mydata, char *send_name)
{
    Node_t *ptmp = pmail->phead;
    pthread_t mytid = pthread_self();

    while (ptmp)
    {
        if (ptmp->data.tid == mytid)
        {
            break;
        }
        ptmp = ptmp->pnext;
    }
    if (NULL == ptmp)
    {
        printf("Not have this thread\n");
        return -1;
    }
    int ret = 0;
    struct msg qdata;
    sem_wait(&ptmp->data.sem);
    pthread_mutex_lock(&ptmp->data.mutex);
    ret = goout_qlink(ptmp->data.pque, &qdata);
    if (-1 == ret)
    {
        pthread_mutex_unlock(&ptmp->data.mutex);
        return -1;
    }
    pthread_mutex_unlock(&ptmp->data.mutex);

    strcpy(send_name, qdata.send_name);
    *mydata = qdata.data;

    return 0;
}
