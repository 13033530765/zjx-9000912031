#ifndef __ATK_M750_H
#define __ATK_M750_H

#include "sys.h"
#include "RingBuffer.h"
#include "string.h"
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "uart2.h"
#include "lcd.h"
#include "mpu6050.h"

#define DTU_NETDATA_RX_BUF (1024)
#define  BUFF_UNIT           512    //缓冲区长度
#define  R_NUM               8     //接收缓冲区个数
#define  T_NUM               8     //发送缓冲区个数  
#define  C_NUM               8     //命令缓冲区个数


//#define cell_phone_number 13396422360

#define  PRODUCTKEY           "a1dWcp4lRBu"                                 //产品ID
#define  PRODUCTKEY_LEN       strlen(PRODUCTKEY)                            //产品ID长度
#define  DEVICENAME           "a1dWcp4lRBu"                                        //设备名  
#define  DEVICENAME_LEN       strlen(DEVICENAME)                            //设备名长度
#define  DEVICESECRE          "dee0ce25d3b6542b47ce09c7500af3ed"            //设备秘钥   

#define  DEVICESECRE_LEN      strlen(DEVICESECRE)                           //设备秘钥长度
#define  S_TOPIC_NAME         "/sys/a1dWcp4lRBu/a1dWcp4lRBu/thing/service/property/set"   //需要订阅的主题  
#define  P_TOPIC_NAME         "/sys/a1dWcp4lRBu/a1dWcp4lRBu/thing/event/property/post"    //需要发布的主题     

#define DTU_SMS_SEND_BUF_MAX    (1024)

#define DTU_RX_CMD_BUF_SIZE (1024)
char cell_phone_number[11]={"15258584491"};
typedef enum
{
	DTU_WORKMODE_NET = 0,  /*网络透传模式*/
	DTU_WORKMODE_HTTP,     /*http透传模式*/
	DTU_WORKMODE_MQTT,     /*mqtt透传模式*/
	DTU_WORKMODE_ALIYUN,   /*阿里云透传模式*/
	DTU_WORKMODE_ONENET,   /*OneNET透传模式*/
    DTU_WORKMODE_BAIDUYUN, /*百度云透传模式*/
} _dtu_work_mode_eu;

typedef struct
{
	uint32_t timeout; /*指令等待超时时间，单位：100ms*/
	char *read_cmd;   /*查询参数指令      请参考DTU AT用户手册说明进行填写*/
	char *write_cmd;  /*配置参数指令      请参考DTU AT用户手册说明进行填写*/
} _dtu_atcmd_st;

void dtu_get_urc_info(uint8_t ch);

void send_data_to_dtu(uint8_t *data, uint32_t size);

int dtu_config_init(_dtu_work_mode_eu work_mode);


int dtu_enter_transfermode(void);
int dtu_enter_configmode(void);

void TxDataBuf_Deal(unsigned char *data, int size);
void MQTT_PublishQs0(char *topic, char *data, int data_len);
void AliIoT_processing_data(void);
int dtu_send_sms(char *phone, char *sms_msg);
int dtu_base_station_orientation(void);
void dtu_base_station_orientation_data_processing(void);


#endif
