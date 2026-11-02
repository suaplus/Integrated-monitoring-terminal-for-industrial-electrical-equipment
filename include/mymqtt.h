#ifndef __MYMQTT_H__
#define __MYMQTT_H__


#include <MQTTAsync.h>
#include <MQTTClient.h>

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include <string.h>
//#define OLD_ADDRESS     "tcp://218.201.45.7:1883"
#define NEW_ADDRESS     "tcp://183.230.40.96:1883"
#define DEV_NAME    "MQTT" //"car_device"   //"device_name"  //"first_device"
#define CLIENTID DEV_NAME
#define PRODUCT_ID    "M47Px4GKHR"  //"y9xVorfvq9"  //"63fg7NqRxY"  //"3XoV8495r6"
#define PASSWD    "version=2018-10-31&res=products%2FM47Px4GKHR%2Fdevices%2FMQTT&et=111111111111112&method=md5&sign=JyfcxSifLkdC4jkPdFBLPg%3D%3D" 
//"version=2018-10-31&res=products%2Fy9xVorfvq9%2Fdevices%2Fcar_device&et=5937255523&method=md5&sign=cUBWLaZDo1t0QSLdpVlXAQ%3D%3D"
//"version=2018-10-31&res=products%2F63fg7NqRxY%2Fdevices%2Fdevice_name&et=3937255523&method=md5&sign=oCu6NsiOgu2LWSV8HnjYUA%3D%3D"
//"version=2018-10-31&res=products%2F3XoV8495r6%2Fdevices%2Ffirst_device&et=1837255523&method=sha1&sign=dtVQufcgwCrn5zmdmWLXFEtJAQY%3D";
#define QOS         0
#define TIMEOUT     10000L



extern int mqtt_init();      // 只创建客户端/设置回调，不联网、不退出
extern int mqtt_connect();   // 尝试连接一次，成功 0 / 失败 -1
extern int mqtt_send(char * key, float value);
extern void mqtt_deinit();

//#define __cplusplus
#endif // HEAD_H
