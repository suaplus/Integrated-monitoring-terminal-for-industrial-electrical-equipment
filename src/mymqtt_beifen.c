#include <stdio.h>
#include "mymqtt.h"

static char topic[2][200] = {0};
static MQTTClient client;
static int id =10000;
volatile static MQTTClient_deliveryToken deliveredtoken;

void pack_topic(char * dev_name, char * pro_id)
{
	sprintf(topic[0], "$sys/%s/%s/thing/property/post/reply", pro_id, dev_name);	//订阅
	sprintf(topic[1], "$sys/%s/%s/thing/property/post", pro_id, dev_name);			//发布
}

// 发送成功后callback
void delivered(void *context, MQTTClient_deliveryToken dt)
{
	printf("Message with token value %d delivery confirmed\n", dt);
	deliveredtoken = dt;
}
// 接收到消息的callback
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
	int i;
	char* payloadptr;

	printf("Message arrived\n");
	printf("     topic: %s\n", topicName);
	printf("   message: ");

	payloadptr = (char*)message->payload;
	for(i=0; i<message->payloadlen; i++)
	{
		putchar(*payloadptr++);
	}
	putchar('\n');
	MQTTClient_freeMessage(&message);
	MQTTClient_free(topicName);
	return 1;
}
// 掉线后的callback
void connlost(void *context, char *cause)
{
	printf("\nConnection lost\n");
	printf("     cause: %s\n", cause);
}

// 只创建客户端、设置回调，不做网络连接、绝不 exit。
// 连接失败是网络问题，不能把整个进程（包括界面）带走。
int mqtt_init()
{
	pack_topic(DEV_NAME, PRODUCT_ID);
    int rc = MQTTClient_create(&client, NEW_ADDRESS, CLIENTID,
                               MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if(MQTTCLIENT_SUCCESS != rc)
    {
        printf("create mqtt client failre...\n");
        client = NULL;
        return -1;
    }

    rc = MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);
    if(MQTTCLIENT_SUCCESS !=rc )
    {
        printf("Failed to set callbacks, return code %d\n", rc);
        return -1;
    }

	return 0;
}

// 尝试连接一次：成功返回 0，失败返回 -1（由调用线程决定何时重试）
int mqtt_connect()
{
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;

    if(NULL == client)
    {
        return -1;
    }

    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.username=PRODUCT_ID;
    conn_opts.password=PASSWD;

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("[mqtt] connect %s failed, return code %d\n", NEW_ADDRESS, rc);
        return -1;
    }

    printf("[mqtt] connected to %s\n", NEW_ADDRESS);
    return 0;
}

int mqtt_send(char * key, float value)
{
	MQTTClient_deliveryToken deliveryToken;
    MQTTClient_message test2_pubmsg = MQTTClient_message_initializer;
    // 需要发送的正文
    char message[1024]={0};

    if(NULL == client)
    {
        return -1;
    }

    test2_pubmsg.qos = QOS;
    test2_pubmsg.retained = 0;
    test2_pubmsg.payload =message;

	sprintf(message,"{\"id\":\"%d\",\"version\":\"1.0\",\"params\":{\"%s\":{\"value\":%f}}}",id, key, value);
	test2_pubmsg.payloadlen = strlen(message);

	printf("%s\n",message);
	int rc = MQTTClient_publishMessage(client,topic[1],&test2_pubmsg,&deliveryToken);
	if(MQTTCLIENT_SUCCESS != rc)
	{
		// 大概率是掉线了，告诉调用线程去重连，不能 exit
		printf("client to publish failure.. rc=%d\n", rc);
		return -1;
	}
	printf("Waiting for up to %d seconds for publication on topic %s for client with ClientID: %s\n"
			,(int)(TIMEOUT/1000), topic[0], CLIENTID);
	MQTTClient_waitForCompletion(client,deliveryToken,TIMEOUT);
	sleep(1);

	return rc;
}

void mqtt_deinit()
{
    if(NULL != client)
    {
        MQTTClient_disconnect(client, 10000);
        MQTTClient_destroy(&client);
        client = NULL;
    }
}

/*
int main(void)
{
	int id = 1;

	mqtt_init();
    while(1)
    {
        mqtt_send("id", id++);
        mqtt_send("temp", rand()%50*0.1);
        mqtt_send("hum", rand()%100*0.1);
 
        //mqtt_send("tmp", 20);
    }
    mqtt_deinit();
	return 0;
}
*/

