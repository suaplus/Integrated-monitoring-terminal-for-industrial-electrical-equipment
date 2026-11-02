#include<stdlib.h>
#include<stdio.h>

#include"qlink.h"

Qlink_t *creat_queue()
{
    Qlink_t *pqlink = malloc(sizeof(Qlink_t));
    if(pqlink == NULL)
    {
        printf("malloc error!\n");
        return NULL;
    }

    pqlink->phead = NULL;
    pqlink->ptail = NULL;
    pqlink->clen = 0;

    return pqlink;
}

int is_empty_queue(Qlink_t *pqlink)
{
    if(pqlink->phead == NULL)
    {
        return 1;
    }
    return 0;
}

int insert_qnode(Qlink_t *pqlink, Data_t data)
{
    Qnode_t *pinsert = malloc(sizeof(Qnode_t));
    if(pinsert == NULL)
    {
        printf("malloc error!\n");
        return -1;
    }
    pinsert->data = data;
    pinsert->pnext = NULL;


    if(is_empty_queue(pqlink))
    {
        pqlink->phead = pinsert;
        pqlink->ptail = pinsert;
        pqlink->clen++;
        return 0;
    }

    pqlink->ptail->pnext = pinsert;
    pqlink->ptail = pinsert;

    pqlink->clen++;

    return 0;
}

int show_qlink(Qlink_t *pqlink)
{
    if(is_empty_queue(pqlink))
    {
        return 0;
    }

    Qnode_t *ptmp = pqlink->phead;

    while(ptmp != NULL)
    {
        //printf("%d ", ptmp->data);
        ptmp = ptmp->pnext;
    }
    printf("\n");
}

int goout_qlink(Qlink_t *pqlink, Data_t *pdata)
{
    if(is_empty_queue(pqlink))
    {
        return -1;
    }

    Qnode_t *pout = pqlink->phead;
    pqlink->phead = pout->pnext;
    *pdata = pout->data;
    free(pout);
    pqlink->clen--;

    if(pqlink->phead == NULL)
    {
        pqlink->ptail = NULL;
    }

    return 0;
}

int del_qlink(Qlink_t *pqlink)
{
    if(is_empty_queue(pqlink))
    {
        return -1;
    }

    Qnode_t *pout = pqlink->phead;
    pqlink->phead = pout->pnext;
    free(pout);

    pqlink->clen--;

    if(pqlink->phead == NULL)
    {
        pqlink->ptail = NULL;
    }

    return 0;
}

int get_head_qlink(Qlink_t *pqlink, Data_t *pdata)
{
    if(is_empty_queue(pqlink))
    {
        return -1;
    }
    *pdata = pqlink->phead->data;

    return 0;
}

int destroy_qlink(Qlink_t *pqlink)
{
    while(pqlink->phead)
    {
        del_qlink(pqlink);
    }
    free(pqlink);
    return 0;
}