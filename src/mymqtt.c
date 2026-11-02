// mymqtt.c —— OneNET MQTT 上传（整帧一次发布版）
//
// 和旧版相比的改动：
//   1. mqtt_init 只建客户端 + 设回调；连接交给 mqtt_connect（可重试、可反复调用）
//   2. 新增 mqtt_pub_json()：一帧数据拼成一个 JSON 一次发出去，不再逐字段发 8 次
//   3. 新增 mqtt_next_id()：OneJSON 的 id 自增（原来恒为 10000）
//   4. 发布前检查连接，掉线就地重连；连不上只丢这一帧，绝不 exit
//   5. 去掉了每次发送末尾的 sleep(1)
//
// 建议在 mymqtt.h 里补上这几行（不补也行，main.c 里已有 extern 声明）：
//   int  mqtt_init(void);
//   int  mqtt_connect(void);
//   int  mqtt_pub_json(const char *json);
//   int  mqtt_next_id(void);
//   void mqtt_deinit(void);

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "mymqtt.h"

static char topic[2][200] = {0};
static MQTTClient client = NULL;
static int msg_id = 10000;
volatile static MQTTClient_deliveryToken deliveredtoken;

void pack_topic(char * dev_name, char * pro_id)
{
	sprintf(topic[0], "$sys/%s/%s/thing/property/post/reply", pro_id, dev_name);	//订阅
	sprintf(topic[1], "$sys/%s/%s/thing/property/post", pro_id, dev_name);			//发布
}

// 发送成功后callback
void delivered(void *context, MQTTClient_deliveryToken dt)
{
	(void)context;
	deliveredtoken = dt;
}

// 接收到消息的callback
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
	int i;
	char* payloadptr;

	(void)context;
	(void)topicLen;

	printf("[mqtt] <- %s : ", topicName);
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
// 这里只打日志，不在这里调 MQTTClient_connect：
// 回调跑在 Paho 内部线程，阻塞式重连容易和库自身的锁打架。
// 重连统一放在 mqtt_pub_json 里，用 MQTTClient_isConnected 判断后被动重连。
void connlost(void *context, char *cause)
{
	(void)context;
	printf("\nConnection lost\n");
	printf("     cause: %s\n", cause ? cause : "unknown");
}

// 只创建客户端、设置回调，不做网络连接、绝不 exit。
// 连接失败是网络问题，不能把整个进程（包括界面）带走。
int mqtt_init()
{
	int rc;

	if (NULL != client)			// 重复调用直接返回，避免 create 两次把前一个 handle 漏掉
	{
		return 0;
	}

	pack_topic(DEV_NAME, PRODUCT_ID);
	rc = MQTTClient_create(&client, NEW_ADDRESS, CLIENTID,
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
		MQTTClient_destroy(&client);
		client = NULL;
		return -1;
	}

	return 0;
}

// 尝试连接一次：成功返回 0，失败返回 -1（由调用线程决定何时重试）
// 已连接时直接返回 0，可被反复调用。
int mqtt_connect()
{
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	int rc;

	if(NULL == client)
	{
		return -1;
	}

	if (MQTTClient_isConnected(client))
	{
		return 0;
	}

	conn_opts.keepAliveInterval = 60;
	conn_opts.cleansession = 1;
	conn_opts.connectTimeout = 5;		// 别让上传线程卡死在 connect 上
	conn_opts.username = PRODUCT_ID;
	conn_opts.password = PASSWD;

	if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
	{
		printf("[mqtt] connect %s failed, return code %d\n", NEW_ADDRESS, rc);
		return -1;
	}

	printf("[mqtt] connected to %s\n", NEW_ADDRESS);

	/* 订阅 property/post/reply，平台的返回码就能在终端看到，方便排错 */
	MQTTClient_subscribe(client, topic[0], QOS);

	return 0;
}

// OneJSON 报文里的消息序号，必须自增，否则平台日志里全是同一个 id
int mqtt_next_id(void)
{
	return ++msg_id;
}

// 整帧一次发布：json 由调用方拼好（OneJSON 格式）
// 返回 0 表示已提交，-1 表示失败（调用方丢弃这一帧即可，不要重试阻塞）
int mqtt_pub_json(const char *json)
{
	MQTTClient_deliveryToken deliveryToken;
	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	int rc;

	if(NULL == client || NULL == json)
	{
		return -1;
	}

	// 掉线就地重连一次；连不上就放弃这一帧，绝不 exit、也不 sleep
	if (!MQTTClient_isConnected(client))
	{
		printf("[mqtt] not connected, reconnecting...\n");
		if (0 != mqtt_connect())
		{
			return -1;
		}
	}

	pubmsg.qos = QOS;
	pubmsg.retained = 0;
	pubmsg.payload = (void *)json;
	pubmsg.payloadlen = strlen(json);

	rc = MQTTClient_publishMessage(client, topic[1], &pubmsg, &deliveryToken);
	if(MQTTCLIENT_SUCCESS != rc)
	{
		printf("[mqtt] publish failure.. rc=%d\n", rc);
		return -1;
	}

	MQTTClient_waitForCompletion(client, deliveryToken, TIMEOUT);

	return 0;
}

// 兼容旧接口：单字段发布。内部走 mqtt_pub_json，
// 并且去掉了原来的 sleep(1) —— 一帧发 8 个字段就是 8 秒，队列必然堆积。
int mqtt_send(char * key, float value)
{
	char message[256];

	if(NULL == client)
	{
		return -1;
	}

	snprintf(message, sizeof(message),
	         "{\"id\":\"%d\",\"version\":\"1.0\",\"params\":{\"%s\":{\"value\":%f}}}",
	         mqtt_next_id(), key, value);

	return mqtt_pub_json(message);
}

void mqtt_deinit()
{
	if(NULL != client)
	{
		if (MQTTClient_isConnected(client))
		{
			MQTTClient_disconnect(client, 1000);
		}
		MQTTClient_destroy(&client);
		client = NULL;
	}
}
