#include <stdio.h>
#include <stdlib.h>
#include "mylink.h"

Link_t *creat_link()
{
    Link_t *link_p = malloc(sizeof(Link_t));
    if(link_p == NULL)
    {
        printf("malloc error!\n");
        return NULL;        
    }

    link_p->phead = NULL;
    link_p->clen = 0;

    return link_p;
}

int is_empty_link(Link_t *plink)
{
    if(plink->phead == NULL)
    {
        return 1;        
    }
    return 0;
}

Node_t *creat_node(LData_t data)
{
    Node_t *pnode = malloc(sizeof(Node_t));
    if(pnode == NULL)
    {
        printf("malloc error!\n");
        return NULL;        
    }

    pnode->data = data;
    pnode->pnext = NULL;

    return pnode;
}

int insert_head_link(Link_t *plink, LData_t data)
{
    Node_t *pnode = creat_node(data);

    pnode->pnext = plink->phead;

    plink->phead = pnode;
    plink->clen++;

    return 0;
}

void show_link(Link_t *plink)
{
    Node_t *ptmp = plink->phead;
    while(ptmp != NULL)
    {
        //printf("%d ", ptmp->data);
        ptmp = ptmp->pnext;     
    }
    printf("\n");
}

int insert_end_link(Link_t *plink, LData_t data)
{
    Node_t *pnode = creat_node(data);

    if(is_empty_link(plink))
    {
        plink->phead = pnode;
        pnode->data = data;
    }else
    {
        Node_t *ptmp = plink->phead;
        while(ptmp->pnext != NULL)
        {
            ptmp = ptmp->pnext;     
        }

        ptmp->pnext = pnode;    
    }
    plink->clen++;

    return 0;
}

int del_head_link(Link_t *plink)
{
    if(is_empty_link(plink))
    {
        return 0;
    }
    Node_t *ptmp = plink->phead;
    plink->phead = plink->phead->pnext;
    free(ptmp);
    plink->clen--;        
    return 0;
}

int del_end_link(Link_t *plink)
{
    if(is_empty_link(plink))
    {
        return 0;
    }   

    if(plink->clen == 1)
    {
        del_head_link(plink);
        return 0;
    }

    Node_t *ptmp = plink->phead;
    while(ptmp->pnext->pnext != NULL)
    {
        ptmp = ptmp->pnext;     
    }
    free(ptmp->pnext);
    ptmp->pnext = NULL;

    plink->clen--;        
    
    return 0;
}

void destroy_link(Link_t *plink)
{
    while(!is_empty_link(plink))
    {
        del_head_link(plink);        
    }

    free(plink);

}