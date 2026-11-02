#include "mq2.h"
 
/*
* @description : 读取指定文件内容
* @param – filename : 要读取的文件路径
* @param - str : 读取到的文件字符串
* @return : 0 成功;其他 失败
*/
int file_data_read(char *filename, char *str)
{
    int ret = 0;
    FILE *data_stream;

    data_stream = fopen(filename, "r"); /* 只读打开 */
    if(data_stream == NULL) {
        printf("can't open file %s\r\n", filename);
        return -1;
    }

    ret = fscanf(data_stream, "%s", str);
    if(!ret) {
        printf("file read error!\r\n");
    } else if(ret == EOF) {
    /* 读到文件末尾的话将文件指针重新调整到文件头 */
        fseek(data_stream, 0, SEEK_SET); 
    }
    fclose(data_stream); /* 关闭文件 */ 
    return 0;
}
 
/*
* @description : 获取 ADC 数据
* @param - dev : 设备结构体
* @return : 0 成功;其他 失败
*/
int adc_read(struct adc_dev *dev)
{
    int ret = 0;
    char str[50];

    SENSOR_FLOAT_DATA_GET(ret, IN_VOLTAGE_SCALE, str, scale);
    SENSOR_INT_DATA_GET(ret, IN_VOLTAGE_RAW, str, raw);
 
    /* 转换得到实际电压值 mV */
    dev->act = (dev->scale * dev->raw)/1000.f;
    return ret;
}
 
/*
* @description : main 主程序
* @param – argc : argv 数组元素个数
* @param - argv : 具体参数
* @return : 0 成功;其他 失败
*/
/*
int main(int argc, char *argv[])
{
    int ret = 0;
    float R0 = 0;
	float Rs = 0;
	float ppmVal = 0;

    if (argc != 1) {
        printf("Error Usage!\r\n");
        return -1;
    }

    while (1) 
    {
        ret = adc_read(&imx6ulladc);
        // 数学曲线拟合公式 实际电压转换气体浓度
		R0 = 6.64;
        Rs = (5 - imx6ulladc.act) / imx6ulladc.act * 0.5;
        ppmVal = pow(11.5428 * R0 / Rs, 0.6549f) * 100;

        if(ret == 0) 
        { // 数据读取成功 
            printf("ADC 原始值：%d，电压值：%.3fV 烟雾浓度:%.2fppm\r\n", imx6ulladc.raw, imx6ulladc.act, ppmVal);
        }
        usleep(100000); //100ms 
    }
    return 0;
}
*/