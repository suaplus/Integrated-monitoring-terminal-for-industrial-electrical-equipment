#ifndef __MAILBOX_H__
#define __MAILBOX_H__

#include "mylink.h"

extern Link_t *mailbox_create_system();
extern int register_thread_mailbox(Link_t *pmail, char *name, void *(*pfun)(void *));
extern void destroy_mailbox(Link_t *pmail);
extern int send_mail_msg(char *name_send, struct msgdata data, char *name_recv);
extern int recv_mail_msg(struct msgdata *data, char *send_name);

#endif