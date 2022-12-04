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
#define  BUFF_UNIT           512    //����������
#define  R_NUM               8     //���ջ���������
#define  T_NUM               8     //���ͻ���������  
#define  C_NUM               8     //�����������


//#define cell_phone_number 13396422360

#define  PRODUCTKEY           "a1dWcp4lRBu"                                 //��ƷID
#define  PRODUCTKEY_LEN       strlen(PRODUCTKEY)                            //��ƷID����
#define  DEVICENAME           "a1dWcp4lRBu"                                        //�豸��  
#define  DEVICENAME_LEN       strlen(DEVICENAME)                            //�豸������
#define  DEVICESECRE          "dee0ce25d3b6542b47ce09c7500af3ed"            //�豸��Կ   

#define  DEVICESECRE_LEN      strlen(DEVICESECRE)                           //�豸��Կ����
#define  S_TOPIC_NAME         "/sys/a1dWcp4lRBu/a1dWcp4lRBu/thing/service/property/set"   //��Ҫ���ĵ�����  
#define  P_TOPIC_NAME         "/sys/a1dWcp4lRBu/a1dWcp4lRBu/thing/event/property/post"    //��Ҫ����������     

#define DTU_SMS_SEND_BUF_MAX    (1024)

#define DTU_RX_CMD_BUF_SIZE (1024)
char cell_phone_number[11]={"15258584491"};
typedef enum
{
	DTU_WORKMODE_NET = 0,  /*����͸��ģʽ*/
	DTU_WORKMODE_HTTP,     /*http͸��ģʽ*/
	DTU_WORKMODE_MQTT,     /*mqtt͸��ģʽ*/
	DTU_WORKMODE_ALIYUN,   /*������͸��ģʽ*/
	DTU_WORKMODE_ONENET,   /*OneNET͸��ģʽ*/
    DTU_WORKMODE_BAIDUYUN, /*�ٶ���͸��ģʽ*/
} _dtu_work_mode_eu;

typedef struct
{
	uint32_t timeout; /*ָ��ȴ���ʱʱ�䣬��λ��100ms*/
	char *read_cmd;   /*��ѯ����ָ��      ��ο�DTU AT�û��ֲ�˵��������д*/
	char *write_cmd;  /*���ò���ָ��      ��ο�DTU AT�û��ֲ�˵��������д*/
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
