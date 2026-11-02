#include "mysqlite3.h"
#include <stdio.h>
#include <sqlite3.h>

int sqlite3_init()
{
    sqlite3* db;
    int ret = sqlite3_open("./parameter_info.db",&db);
    if(SQLITE_OK != ret)
    {
        fprintf(stderr,"open error:%s\n",sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
       
    char sql_cmd1[512]="create table info(id INT, name TEXT, age INT);";//table name  info

    ret = sqlite3_exec(db,sql_cmd1,NULL,NULL,NULL);


    char sql_cmd[512]="insert into info values(10,'wangmazi',20);";//table name  info
    char * errmsg;
    ret = sqlite3_exec(db,sql_cmd,NULL,NULL,&errmsg);
    if(SQLITE_OK != ret)
    {
        fprintf(stderr,"exec error:%s\n",errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        return 1;
    }
    sqlite3_close(db);

    return 0;
}