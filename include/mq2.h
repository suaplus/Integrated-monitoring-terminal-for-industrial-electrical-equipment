#ifndef __MQ2_H__
#define __MQ2_H__

#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "sys/ioctl.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include <poll.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>

//12位ADC

/* 字符串转数字，将浮点小数字符串转换为浮点数数值 */
#define SENSOR_FLOAT_DATA_GET(ret, index, str, member)\
ret = file_data_read(file_path[index], str);\
dev->member = atof(str);\
 
/* 字符串转数字，将整数字符串转换为整数数值 */
#define SENSOR_INT_DATA_GET(ret, index, str, member)\
ret = file_data_read(file_path[index], str);\
dev->member = atoi(str);\

/* adc iio 框架对应的文件路径 */
static char *file_path[] = {
"/sys/bus/iio/devices/iio:device0/in_voltage_scale",
"/sys/bus/iio/devices/iio:device0/in_voltage1_raw",
};

/* 文件路径索引，要和 file_path 里面的文件顺序对应 */
enum path_index {
IN_VOLTAGE_SCALE = 0,
IN_VOLTAGE_RAW,
};

/*
* ADC 数据设备结构体
*/
struct adc_dev{
    int raw;
    float scale;
    float act;
};

extern int file_data_read(char *filename, char *str);
extern int adc_read(struct adc_dev *dev);

#endif