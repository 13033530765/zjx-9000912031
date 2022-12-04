#include "atk_m750.h"

unsigned char  MQTT_RxDataBuf[R_NUM][BUFF_UNIT];            //���ݵĽ��ջ�����,���з��������������ݣ�����ڸû�����,��������һ���ֽڴ�����ݳ���
unsigned char *MQTT_RxDataInPtr;                            //ָ����ջ�����������ݵ�λ��
unsigned char *MQTT_RxDataOutPtr;                           //ָ����ջ�������ȡ���ݵ�λ��
unsigned char *MQTT_RxDataEndPtr;                           //ָ����ջ�����������λ��

unsigned char  MQTT_TxDataBuf[T_NUM][BUFF_UNIT];            //���ݵķ��ͻ�����,���з��������������ݣ�����ڸû�����,��������һ���ֽڴ�����ݳ���
unsigned char *MQTT_TxDataInPtr;                            //ָ���ͻ�����������ݵ�λ��
unsigned char *MQTT_TxDataOutPtr;                           //ָ���ͻ�������ȡ���ݵ�λ��
unsigned char *MQTT_TxDataEndPtr;                           //ָ���ͻ�����������λ��

unsigned char  MQTT_CMDBuf[C_NUM][BUFF_UNIT];               //�������ݵĽ��ջ�����
unsigned char *MQTT_CMDInPtr;                               //ָ���������������ݵ�λ��
unsigned char *MQTT_CMDOutPtr;                              //ָ�����������ȡ���ݵ�λ��
unsigned char *MQTT_CMDEndPtr;                              //ָ���������������λ��

int   Fixed_len;                       					     //�̶���ͷ����
int   Variable_len;                     					 //�ɱ䱨ͷ����
int   Payload_len;                       					 //��Ч���ɳ���
unsigned char  temp_buff[BUFF_UNIT];						 //��ʱ������������������

extern RingBuffer *p_uart2_rxbuf;
extern u8 dtu_state;
uint8_t dtu_rxcmdbuf[DTU_RX_CMD_BUF_SIZE]; /*����DTU������ݻ���*/
uint32_t dtu_rxlen = 0;
uint8_t dtu_rxbuf[DTU_NETDATA_RX_BUF];

/*���Ա���*/
u8 current;
u8 longitude_degreefen=0,latitude_degreefen=0;//���ȣ��ȡ�γ�ȣ���
u32 longitude_minute=0,latitude_minute=0;//���ȣ��֡�γ�ȣ���
extern u8 PitchVPT,RollVPT;
extern float Pitch,Roll;
char p[128],identifier[128];
extern u8 MPU6050_Pitch_y,MPU6050_Roll_y;
extern u8 set_PitchVPT_log,set_RollVPT_log;
char cell_phone_number[11]={"15388908342"};




/**
 * @brief       �������ݵ�DTU
 * 
 * @param       data:   ��Ҫ�������ݵĻ����ַ
 * @param       size:   �������ݴ�С
 * 
 * @return      ��
 * 
*/
void send_data_to_dtu(uint8_t *data, uint32_t size)
{
    usart2_send_data(data, size);
}

/**
 * @brief       �������DTU����������У��
 * 
 * @param       cmd     :   ��Ҫ���͵�ATָ��
 * @param       ask     :   ��ҪУ���Ӧ������
 * @param       timeout :   ATָ��У�鳬ʱʱ�䣬��λ��100ms
 * 
 * @return      1  :   У��ask���ݳɹ�
 *              0  :   DTU����OK
 *             -1  :   DTU����ERROR
 *             -2  :   ����ATָ��У�鳬ʱ
 */
static int send_cmd_to_dtu(char *cmd, char *ask, uint32_t timeout)
{
    uint32_t rx_len = 0;

    /*��ʼ����������*/
    memset(dtu_rxcmdbuf, 0, DTU_RX_CMD_BUF_SIZE);
    RingBuffer_Reset(p_uart2_rxbuf);

    /*����ATָ�DTU*/
    send_data_to_dtu((uint8_t *)cmd, strlen(cmd));

    /*�ȴ�DTUӦ��ATָ����*/
    while (1)
    {
        if (strstr((char *)dtu_rxcmdbuf, ask) != NULL)
        {
            return 1;
        }
        else if (strstr((char *)dtu_rxcmdbuf, "OK") != NULL)
        {
            return 0;
        }
        else if (strstr((char *)dtu_rxcmdbuf, "ERROR") != NULL)
        {
            return -1;
        }

        if (RingBuffer_Len(p_uart2_rxbuf) > 0)
        {
            RingBuffer_Out(p_uart2_rxbuf, &dtu_rxcmdbuf[rx_len++], 1); //�Ӵ��ڻ����ж�һ���ֽ�

            if (rx_len >= DTU_RX_CMD_BUF_SIZE) /*����Ӧ�����ݳ���������ERROR*/
            {
                return -1;
            }
        }
        else
        {
            timeout--;

            if (timeout == 0)
            {
                return -2;
            }

            delay_ms(100);
        }
    }
}

/**
 * @brief       DTU��������״̬
 * 
 * @param       ��
 * 
 * @return      0  :    �ɹ���������״̬
 *             -1  :    ��������״̬ʧ��
 */
int dtu_enter_configmode(void)
{
    int res;

    /* 1.����+++׼����������״̬ */
    res = send_cmd_to_dtu("+++", "atk", 5);
    if (res == -1) /*����ERRRO��ʾDTU�Ѿ���������״̬*/
    {	
			dtu_state=0;
        return 0;
    }

    /* 2.����atkȷ�Ͻ�������״̬ */
    res = send_cmd_to_dtu("atk", "OK", 5);
    if (res == -2)
    {
        return -1;
    }
		dtu_state=0;
    return 0;
}

/**
 * @brief       DTU����͸��״̬
 * 
 * @param       ��
 * 
 * @return      0  :    �ɹ�����͸��״̬
 *             -1  :    ����͸��״̬ʧ��
 */
int dtu_enter_transfermode(void)
{
    if (send_cmd_to_dtu("ATO\r\n", "OK", 5) >= 0)
    {
        return 0;
    }

    return -1;
}

/**
 * @brief       DTU�Զ��ϱ�URC��Ϣ��������:����+ATK ERROR��Ϣ
 * 
 * @param       data    :   ���յ�DTU��URC���ݻ���
 * @param       len     :   URC���ݳ���
 * 
 * @return      ��
 */
static void dtu_urc_atk_error(const char *data, uint32_t len)
{
//    printf("\r\nURC :   dtu_urc_atk_error\r\n");
}

/**
 * @brief       DTU�Զ��ϱ�URC��Ϣ��������:����Please check SIM Card��Ϣ
 * 
 * @param       data    :   ���յ�DTU��URC���ݻ���
 * @param       len     :   URC���ݳ���
 * 
 * @return      ��
 */
static void dtu_urc_error_sim(const char *data, uint32_t len)
{
//    printf("\r\nURC :   dtu_urc_error_sim\r\n");
}

/**
 * @brief       DTU�Զ��ϱ�URC��Ϣ��������:����Please check GPRS��Ϣ
 * 
 * @param       data    :   ���յ�DTU��URC���ݻ���
 * @param       len     :   URC���ݳ���
 * 
 * @return      ��
 */
static void dtu_urc_error_gprs(const char *data, uint32_t len)
{
//    printf("\r\nURC :   dtu_urc_error_gprs\r\n");
}

/**
 * @brief       DTU�Զ��ϱ�URC��Ϣ��������:����Please check CSQ��Ϣ
 * 
 * @param       data    :   ���յ�DTU��URC���ݻ���
 * @param       len     :   URC���ݳ���
 * 
 * @return      ��
 */
static void dtu_urc_error_csq(const char *data, uint32_t len)
{
//    printf("\r\nURC :   dtu_urc_error_csq\r\n");
}

/**
 * @brief       DTU�Զ��ϱ�URC��Ϣ��������:����Please check MQTT Parameter��Ϣ
 * 
 * @param       data    :   ���յ�DTU��URC���ݻ���
 * @param       len     :   URC���ݳ���
 * 
 * @return      ��
 */
static void dtu_urc_error_mqtt(const char *data, uint32_t len)
{
//    printf("\r\nURC :   dtu_urc_error_mqtt\r\n");
}

typedef struct
{
    const char *urc_info;                         /*DTU�Զ��ϱ���URC��Ϣ*/
    void (*func)(const char *data, uint32_t len); /*�ص���������*/
} _dtu_urc_st;

#define DTU_ATK_M750_URC_SIZE 5
static _dtu_urc_st DTU_ATK_M750_URC[DTU_ATK_M750_URC_SIZE] =
    {
       
        {"+ATK ERROR:",                         dtu_urc_atk_error},         /*DTU�������⣬��Ҫ��ϵ����֧�ֽ���ȷ��*/
        {"Please check SIM Card !!!\r\n",       dtu_urc_error_sim},         /*DTUδ��⵽�ֻ���,�����ֻ����Ƿ���ȷ����*/
        {"Please check GPRS !!!\r\n",           dtu_urc_error_gprs},        /*����SIM���Ƿ�Ƿ��*/
        {"Please check CSQ !!!\r\n",            dtu_urc_error_csq},         /*���������Ƿ���ȷ���룬��ȷ������λ�õ���ȷ��*/
        {"Please check MQTT Parameter !!!\r\n", dtu_urc_error_mqtt},        /*MQTT��������*/
};

/**
 * @brief       ����DTU�����ϱ���URC��Ϣ���ݣ�ע�⣺����ÿ����һ���ֽ����ݣ�����Ҫͨ��������ڴ������
 * 
 * @param       ch    :   ���ڽ��յ�һ���ֽ�����
 * 
 * @return      ��
 */
void dtu_get_urc_info(uint8_t ch)
{
    static uint8_t ch_last = 0;
    static uint32_t rx_len = 0;
    int i;

    /*����DTU����*/
    dtu_rxcmdbuf[rx_len++] = ch;
    if (rx_len >= DTU_RX_CMD_BUF_SIZE)
    { /*��������*/
        ch_last = 0;
        rx_len = 0;
        memset(dtu_rxcmdbuf, 0, DTU_RX_CMD_BUF_SIZE);
    }

    /*����DTU��URC����*/
    if ((ch_last == '\r') && (ch == '\n'))
    {
        for (i = 0; i < DTU_ATK_M750_URC_SIZE; i++)
        {
            if (strstr((char *)dtu_rxcmdbuf, DTU_ATK_M750_URC[i].urc_info) == (char *)dtu_rxcmdbuf)
            {
                DTU_ATK_M750_URC[i].func((char *)dtu_rxcmdbuf, strlen((char *)dtu_rxcmdbuf));
            }
        }

        ch_last = 0;
        rx_len = 0;
        memset(dtu_rxcmdbuf, 0, DTU_RX_CMD_BUF_SIZE);
    }

    ch_last = ch;
}

static const _dtu_atcmd_st dtu_net_param_info[] = {

    /*1.ѡ����ģʽΪ������͸��ģʽ*/
    {5, "AT+WORK\r\n",      "AT+WORK=\"NET\"\r\n"},

    /*2.��������͸��ģʽ�Ĺ�������*/
    {5, "AT+LINK1EN\r\n",   "AT+LINK1EN=\"ON\"\r\n"},
    {5, "AT+LINK1\r\n",     "AT+LINK1=\"TCP\",\"cloud.alientek.com\",\"59666\"\r\n"},
    {5, "AT+LINK1MD\r\n",   "AT+LINK1MD=\"LONG\"\r\n"},
    {5, "AT+LINK1TM\r\n",   "AT+LINK1TM=\"5\"\r\n"},

    {5, "AT+LINK2EN\r\n",   "AT+LINK2EN=\"OFF\"\r\n"},
    {5, "AT+LINK3EN\r\n",   "AT+LINK3EN=\"OFF\"\r\n"},
    {5, "AT+LINK4EN\r\n",   "AT+LINK4EN=\"OFF\"\r\n"},

    /*3.����ԭ���ƹ��ܣ�Ĭ�ϴ�*/
    {5, "AT+SVREN\r\n",     "AT+SVREN=\"ON\"\r\n"},
    {5, "AT+SVRNUM\r\n",    "AT+SVRNUM=\"12345678901234567890\"\r\n"},
    {5, "AT+SVRKEY\r\n",    "AT+SVRKEY=\"12345678\"\r\n"},

    /*4.�������������ܣ�Ĭ�ϴ�         ע�⣺ǿ�ҽ��鿪�����������ܣ�����*/
    {5, "AT+HRTEN\r\n",     "AT+HRTEN=\"ON\"\r\n"},
    {5, "AT+HRTDT\r\n",     "AT+HRTDT=\"414C49454E54454B2D4852544454\"\r\n"},
    {5, "AT+HRTTM\r\n",     "AT+HRTTM=\"120\"\r\n"},

    /*5.����ע������ܣ�Ĭ�Ϲر� */
    {5, "AT+REGEN\r\n",     "AT+REGEN=\"OFF\"\r\n"},
    {5, "AT+REGDT\r\n",     "AT+REGDT=\"414C49454E54454B2D5245474454\"\r\n"},
    {5, "AT+REGMD\r\n",     "AT+REGMD=\"LINK\"\r\n"},
    {5, "AT+REGTP\r\n",     "AT+REGTP=\"IMEI\"\r\n"},

    /*6.����������������*/
};

static const _dtu_atcmd_st dtu_http_param_info[] = {

    /*1.ѡ����ģʽΪ��HTTP͸��ģʽ*/
    {5, "AT+WORK\r\n",      "AT+WORK=\"HTTP\"\r\n"},

    /*2.����HTTP͸��ģʽ�Ĺ�������*/
    {5, "AT+HTTPMD\r\n",    "AT+HTTPMD=\"GET\"\r\n"},
    {5, "AT+HTTPURL\r\n",   "AT+HTTPURL=\"https://cloud.alientek.com/testfordtu?data=\"\r\n"},
    {5, "AT+HTTPTM\r\n",    "AT+HTTPTM=\"10\"\r\n"},
    {5, "AT+HTTPHD\r\n",    "AT+HTTPHD=\"Connection:close\"\r\n"},

    /*3.����������������*/
};

static const _dtu_atcmd_st dtu_mqtt_param_info[] = {

    /*1.ѡ����ģʽΪ��MQTT͸��ģʽ*/
    {5, "AT+WORK\r\n",      "AT+WORK=\"MQTT\"\r\n"},

    /*2.����MQTT͸��ģʽ�Ĺ�������*/
    {5, "AT+MQTTCD\r\n",    "AT+MQTTCD=\"alientek\"\r\n"},
    {5, "AT+MQTTUN\r\n",    "AT+MQTTUN=\"admin\"\r\n"},
    {5, "AT+MQTTPW\r\n",    "AT+MQTTPW=\"password\"\r\n"},
    {5, "AT+MQTTIP\r\n",    "AT+MQTTIP=\"cloud.alientek.com\",\"1883\"\r\n"},
    {5, "AT+MQTTSUB\r\n",   "AT+MQTTSUB=\"atk/sub\"\r\n"},
    {5, "AT+MQTTPUB\r\n",   "AT+MQTTPUB=\"atk/pub\"\r\n"},
    {5, "AT+MQTTCON\r\n",   "AT+MQTTCON=\"0\",\"0\",\"1\",\"300\"\r\n"},

    /*3.����������������*/
};

static const _dtu_atcmd_st dtu_aliyun_param_info[] = {

    /*1.ѡ����ģʽΪ��������͸��ģʽ*/
    {5, "AT+WORK\r\n",      "AT+WORK=\"ALIYUN\"\r\n"},

    /*2.����MQTT͸��ģʽ�Ĺ�������*/
    {5, "AT+ALIPK\r\n",     "AT+ALIPK=\"a1dWcp4lRBu\"\r\n"},
    {5, "AT+ALIDS\r\n",     "AT+ALIDS=\"dee0ce25d3b6542b47ce09c7500af3ed\"\r\n"},
    {5, "AT+ALIDN\r\n",     "AT+ALIDN=\"a1dWcp4lRBu\"\r\n"},
    {5, "AT+ALIRI\r\n",     "AT+ALIRI=\"cn-shanghai\"\r\n"},
    {5, "AT+ALISUB\r\n",    "AT+ALISUB=\"/sys/a1dWcp4lRBu/a1dWcp4lRBu/thing/service/property/set\"\r\n"},
    {5, "AT+ALIPUB\r\n",    "AT+ALIPUB=\"/sys/a1dWcp4lRBu/a1dWcp4lRBu/thing/event/property/post\"\r\n"},
    {5, "AT+ALICON\r\n",    "AT+ALICON=\"0\",\"0\",\"1\",\"300\"\r\n"},

    /*3.����������������*/
};

static const _dtu_atcmd_st dtu_onenet_param_info[] = {

    /*1.ѡ����ģʽΪ��OneNET͸��ģʽ*/
    {5, "AT+WORK\r\n",      "AT+WORK=\"ONENET\"\r\n"},

    /*2.����OneNET͸��ģʽ�Ĺ�������*/
    {5, "AT+ONEDI\r\n",     "AT+ONEDI=\"12345\"\r\n"},
    {5, "AT+ONEPI\r\n",     "AT+ONEPI=\"1234567890\"\r\n"},
    {5, "AT+ONEAI\r\n",     "AT+ONEAI=\"12345678901234567890\"\r\n"},
    {5, "AT+ONEIP\r\n",     "AT+ONEIP=\"mqtt.hecloud.com\",\"6002\"\r\n"},
    {5, "AT+ONECON\r\n",    "AT+ONECON=\"1\",\"0\",\"0\",\"1\",\"300\"\r\n"},

    /*3.����������������*/
};

static const _dtu_atcmd_st dtu_baiduyun_param_info[] = {

    /*1.ѡ����ģʽΪ���ٶ���͸��ģʽ*/
    {5, "AT+WORK\r\n",      "AT+WORK=\"BAIDUYUN\"\r\n"},

    /*2.���ðٶ���͸��ģʽ�Ĺ�������*/
    {5, "AT+BAIEP\r\n",     "AT+BAIEP=\"alientek\"\r\n"},
    {5, "AT+BAINM\r\n",     "AT+BAINM=\"name\"\r\n"},
    {5, "AT+BAIKEY\r\n",    "AT+BAIKEY=\"key\"\r\n"},
    {5, "AT+BAIRI\r\n",     "AT+BAIRI=\"gz\"\r\n"},
    {5, "AT+BAISUB\r\n",    "AT+BAISUB=\"sub\"\r\n"},
    {5, "AT+BAIPUB\r\n",    "AT+BAIPUB=\"pub\"\r\n"},
    {5, "AT+BAICON\r\n",    "AT+BAICON=\"0\",\"0\",\"0\",\"1\",\"300\"\r\n"},

    /*3.����������������*/
};

/**
 * @brief       ����DTU��������
 * 
 * @param       work_param      :   ����ģʽ���ATָ�����
 * @param       num             :   ��Ҫ���õ�ATָ���������
 * 
 * @return      0  :    ���в������óɹ�
 *              n  :    �ڼ�����������ʧ�ܣ�1-n
 */
static int dtu_config_work_param(_dtu_atcmd_st *work_param, uint8_t num)
{
    int i;
    int res = 0;

    for (i = 0; i < num; i++)
    {
        res = send_cmd_to_dtu((work_param + i)->read_cmd,
                              (work_param + i)->write_cmd + strlen((work_param + i)->read_cmd) - 1,
                              work_param[i].timeout);

        if (res == 1) /*���DTU�ڲ���������Ҫ���õĲ���һ�£�����Ҫ�ظ�ȥ����*/
        {
            continue;
        }
        else /*DTU�ڲ����������ò�����һ�£���Ҫ����DTU�ڲ�����*/
        {
            res = send_cmd_to_dtu((work_param + i)->write_cmd,
                                  "OK",
                                  (work_param + i)->timeout);

            if (res < 0)
            {
                return i+1;
            }
        }
    }

    return 0;
}

/**
 * @brief       ��ʼ��DTU�Ĺ���״̬
 * 
 * @param       work_mode   :   DTU����ģʽ
 *  @arg        DTU_WORKMODE_NET,       0,  ����͸��ģʽ
 *  @arg        DTU_WORKMODE_HTTP,      1,  http͸��ģʽ
 *  @arg        DTU_WORKMODE_MQTT,      2,  mqtt͸��ģʽ
 *  @arg        DTU_WORKMODE_ALIYUN,    3,  ������͸��ģʽ
 *  @arg        DTU_WORKMODE_ONENET,    4,  OneNET͸��ģʽ
 *  @arg        DTU_WORKMODE_BAIDUYUN,  5,  �ٶ���͸��ģʽ
 * 
 * @return      0   :   ��ʼ���ɹ�
 *             -1   :   ������״̬ʧ��
 *             -2   :   DTU������������ʧ��
 *             -3   ��  DTU����͸��״̬ʧ��
 */
int dtu_config_init(_dtu_work_mode_eu work_mode)
{
    int res;

    /*1.DTU��������״̬*/
    res = dtu_enter_configmode();
    if ( res != 0 )
    {
        return -1;
    }

    /*2.����DTU�Ĺ�������*/
    switch (work_mode)
    {
        case DTU_WORKMODE_NET:
        {
            res = dtu_config_work_param((_dtu_atcmd_st *)&dtu_net_param_info, sizeof(dtu_net_param_info) / sizeof(_dtu_atcmd_st));
            break;
        }
        case DTU_WORKMODE_HTTP:
        {
            res = dtu_config_work_param((_dtu_atcmd_st *)&dtu_http_param_info, sizeof(dtu_http_param_info) / sizeof(_dtu_atcmd_st));
            break;
        }
        case DTU_WORKMODE_MQTT:
        {
            res = dtu_config_work_param((_dtu_atcmd_st *)&dtu_mqtt_param_info, sizeof(dtu_mqtt_param_info) / sizeof(_dtu_atcmd_st));
            break;
        }
        case DTU_WORKMODE_ALIYUN:
        {
            res = dtu_config_work_param((_dtu_atcmd_st *)&dtu_aliyun_param_info, sizeof(dtu_aliyun_param_info) / sizeof(_dtu_atcmd_st));
            break;
        }
        case DTU_WORKMODE_ONENET:
        {
            res = dtu_config_work_param((_dtu_atcmd_st *)&dtu_onenet_param_info, sizeof(dtu_onenet_param_info) / sizeof(_dtu_atcmd_st));
            break;
        }
        case DTU_WORKMODE_BAIDUYUN:
        {
            res = dtu_config_work_param((_dtu_atcmd_st *)&dtu_baiduyun_param_info, sizeof(dtu_baiduyun_param_info) / sizeof(_dtu_atcmd_st));
            break;
        }
        default:
        {
            break;
        }
    }

    if( res != 0 )
    {
        return -2;
    }

    /*3.DTU����͸��״̬*/
    res = dtu_enter_transfermode();
    if( res != 0 )
    {
        return -3;
    }

    return 0;
}

/*----------------------------------------------------------*/
/*���������������ͻ�����                                    */
/*��  ����data������                                        */
/*��  ����size�����ݳ���                                    */
/*����ֵ����                                                */
/*----------------------------------------------------------*/
void TxDataBuf_Deal(unsigned char *data, int size)
{
	memcpy(&MQTT_TxDataInPtr[2],data,size);      //�������ݵ����ͻ�����	
	MQTT_TxDataInPtr[0] = size/256;              //��¼���ݳ���
	MQTT_TxDataInPtr[1] = size%256;              //��¼���ݳ���
	MQTT_TxDataInPtr+=BUFF_UNIT;                 //ָ������
	if(MQTT_TxDataInPtr==MQTT_TxDataEndPtr)      //���ָ�뵽������β����
		MQTT_TxDataInPtr = MQTT_TxDataBuf[0];    //ָ���λ����������ͷ
}


/*----------------------------------------------------------*/
/*���������ȼ�0 ������Ϣ����                                */
/*��  ����topic_name��topic����                             */
/*��  ����data������                                        */
/*��  ����data_len�����ݳ���                                */
/*����ֵ����                                                */
/*----------------------------------------------------------*/
void MQTT_PublishQs0(char *topic, char *data, int data_len)
{	
	int temp,Remaining_len;
	
	Fixed_len = 1;                              //�̶���ͷ������ʱ�ȵ��ڣ�1�ֽ�
	Variable_len = 2 + strlen(topic);           //�ɱ䱨ͷ���ȣ�2�ֽ�(topic����)+ topic�ַ����ĳ���
	Payload_len = data_len;                     //��Ч���ɳ��ȣ�����data_len
	Remaining_len = Variable_len + Payload_len; //ʣ�೤��=�ɱ䱨ͷ����+���س���
	
	temp_buff[0]=0x30;                       //�̶���ͷ��1���ֽ� ���̶�0x30   	
	do{                                      //ѭ�������̶���ͷ�е�ʣ�೤���ֽڣ��ֽ�������ʣ���ֽڵ���ʵ���ȱ仯
		temp = Remaining_len%128;            //ʣ�೤��ȡ��128
		Remaining_len = Remaining_len/128;   //ʣ�೤��ȡ��128
		if(Remaining_len>0)               	
			temp |= 0x80;                    //��Э��Ҫ��λ7��λ          
		temp_buff[Fixed_len] = temp;         //ʣ�೤���ֽڼ�¼һ������
		Fixed_len++;	                     //�̶���ͷ�ܳ���+1    
	}while(Remaining_len>0);                 //���Remaining_len>0�Ļ����ٴν���ѭ��
		             
	temp_buff[Fixed_len+0]=strlen(topic)/256;                      //�ɱ䱨ͷ��1���ֽ�     ��topic���ȸ��ֽ�
	temp_buff[Fixed_len+1]=strlen(topic)%256;		               //�ɱ䱨ͷ��2���ֽ�     ��topic���ȵ��ֽ�
	memcpy(&temp_buff[Fixed_len+2],topic,strlen(topic));           //�ɱ䱨ͷ��3���ֽڿ�ʼ ������topic�ַ���	
	memcpy(&temp_buff[Fixed_len+2+strlen(topic)],data,data_len);   //��Ч���ɣ�����data����
	
	TxDataBuf_Deal(temp_buff, Fixed_len + Variable_len + Payload_len);  //���뷢�����ݻ�����
}


void AliIoT_processing_data(void)
{
	int p_1;
	u16 numerical=0;
	uint8_t buf;
	char temp[256];
	if (RingBuffer_Len(p_uart2_rxbuf) > 0)          /*���յ�DTU���͹����ķ���������*/
  {
		RingBuffer_Out(p_uart2_rxbuf, &buf, 1);
    dtu_rxbuf[dtu_rxlen++] = buf;
    dtu_get_urc_info(buf);                      /*����DTU�ϱ���URC��Ϣ*/
    if (dtu_rxlen >= DTU_NETDATA_RX_BUF)        /*���ջ������*/
    {
//      usart1_send_data(dtu_rxbuf, dtu_rxlen); /*���յ���DTU���������������ݣ�ת�������Դ���1���*/
      dtu_rxlen = 0;
    }
  }
  else
  {
		if (dtu_rxlen>0)
    {
			if(strstr((char *)dtu_rxcmdbuf,"\"params\":{\"PitchVPT\""))//strstr((char *)dtu_rxcmdbuf, ask)
			{//���������"params":{"LightSwitch":1}˵���������·��򿪿���1	
				sprintf(p,"%s",strstr((char *)dtu_rxbuf,"\"params\":{\"PitchVPT\""));
				sprintf(identifier,"%s","PitchVPT");
				p_1=13;
				p_1=p_1+8;//current�ĳ���Ϊ7
				while(p[p_1]!='}')
				{
					numerical=numerical*10+p[p_1]-48;
					p_1++;
				}
				PitchVPT=numerical;
				set_PitchVPT_log=1;
				LCD_Fill(115+5,MPU6050_Pitch_y,115+5+43,MPU6050_Pitch_y+14,WHITE);
				LCD_ShowFloat(115+7,MPU6050_Pitch_y,16,PitchVPT,5,0);
				sprintf(temp,"{\"method\":\"thing.event.property.post\",\"id\":\"203302322\",\"params\":{\"PitchVPT\":%d},\"version\":\"1.0.0\"}",PitchVPT);  //����jw01���ݻظ�
				usart2_send_data((u8 *)temp,strlen(temp));
//				AlibabaCloudIssuedTheLogo=1;
				//�жϿ���״̬����������������  
			}
			else if(strstr((char *)dtu_rxbuf,"\"params\":{\"RollVPT\""))
			{
				sprintf(p,"%s",strstr((char *)dtu_rxbuf,"\"params\":{\"RollVPT\""));
				sprintf(identifier,"%s","RollVPT");
				p_1=13;
				p_1=p_1+7;//current�ĳ���Ϊ7
				while(p[p_1]!='}')
				{
					numerical=numerical*10+p[p_1]-48;
					p_1++;
				}
				RollVPT=numerical;
				set_RollVPT_log=1;
				LCD_Fill(115+5,MPU6050_Roll_y,115+5+43,MPU6050_Roll_y+14,WHITE);
				LCD_ShowFloat(115+7,MPU6050_Roll_y,16,RollVPT,5,0);
				sprintf(temp,"{\"method\":\"thing.event.property.post\",\"id\":\"203302322\",\"params\":{\"RollVPT\":%d},\"version\":\"1.0.0\"}",RollVPT);  //����jw01���ݻظ�
				usart2_send_data((u8 *)temp,strlen(temp));
			}
			delay_ms(10);
//      usart1_send_data(dtu_rxbuf, dtu_rxlen); /*���յ���DTU���������������ݣ�ת�������Դ���1���*/
      dtu_rxlen = 0;
    }
     delay_ms(10);
  }

}

int dtu_send_sms(char *phone, char *sms_msg)//������
{
  static char dtu_sms_buf[1024];

	int res;
  int ret = 0;

    /*1.DTU??????*/
	res = dtu_enter_configmode();
	if ( res != 0 )
	{
		return -1;
	}

	snprintf(dtu_sms_buf, DTU_SMS_SEND_BUF_MAX, "AT+SMSEND=\"%s\",\"%s\"\r\n", phone, sms_msg);

	/* 2.DTU???? */
	res = send_cmd_to_dtu(dtu_sms_buf, "SMSEND OK", 100);
	if( res == 1 )
	{
		printf("���ͳɹ�");
		delay_ms(5000);
		ret = 1;
	}

	/*3.DTU??????*/
	res = dtu_enter_transfermode();
	if( res != 0 )
	{
		return -3;
	}

	return ret;
}
int dtu_base_station_orientation(void)//��վ��λ
{
	uint8_t buf;
  static char dtu_sms_buf[1024];
	int res;
	int ret = 0;
	/*1.DTU??????*/
	res = dtu_enter_configmode();
	if ( res != 0 )
	{
		return -1;
	}
	snprintf(dtu_sms_buf,DTU_SMS_SEND_BUF_MAX,"AT+LOC\r\n");
	/* 2.DTU???? */
	res = send_cmd_to_dtu(dtu_sms_buf,"AT+LOC\r\n", 100);
	if( res == 1 )
	{
		while(1)
		{
			if(RingBuffer_Len(p_uart2_rxbuf) > 0)          /*���յ�DTU���͹����ķ���������*/
			{
				RingBuffer_Out(p_uart2_rxbuf, &buf, 1);
				dtu_rxbuf[dtu_rxlen++] = buf;
				if (dtu_rxlen >= DTU_NETDATA_RX_BUF)        /*���ջ������*/
				{
					dtu_rxlen = 0;
				}
			}
			else
			{
				if (dtu_rxlen > 0)
				{
					if(strstr((char *)dtu_rxbuf,"+LOCATION"))
					{
						dtu_base_station_orientation_data_processing();
						delay_ms(5);
						break;
					}
					else if(strstr((char *)dtu_rxbuf,"ERROR"))
					{
						break;
					}
						dtu_rxlen = 0;
				}
				delay_ms(10);
			}
		}
		delay_ms(10);
	}
	delay_ms(10);
	/*3.DTU??????*/
	res = dtu_enter_transfermode();
	if( res != 0 )
	{
		return -3;
	}
	return ret;
}


void dtu_base_station_orientation_data_processing(void)
{
	u8 dot_log=0,comma_log=0;//���־�����ű�־
	if(strstr((char *)dtu_rxbuf,"+LOCATION"))//+LOCATION:120.168640,35.969391
		//+LOCATION:120.168640,35.969391
	{
		delay_ms(10);
		longitude_degreefen=0;latitude_degreefen=0;//���ȣ��ȡ�γ�ȣ���
		longitude_minute=0;latitude_minute=0;//���ȣ��֡�γ�ȣ���
		sprintf(p,"%s",strstr((char *)dtu_rxbuf,"+LOCATION"));
		for(int i=10;i<32;i++)
		{
			if(p[i]==',')
			{
				comma_log=comma_log+1;
			}
			else if(p[i]=='.')
			{
				dot_log=dot_log+1;
			}
			if(p[i]!=','&&p[i]!='.'&&p[i]!=':'&&p[i]!=' '&&p[i]!='\r')
			{
				if(comma_log==0)
				{
					if(dot_log==0)
					{
						longitude_degreefen=longitude_degreefen*10+p[i]-48;//���ȣ���
					}
					else if(dot_log==1)
					{
						longitude_minute=longitude_minute*10+p[i]-48;//���ȣ���
					}
				}
				else if(comma_log==1)
				{
					if(dot_log==1)
					{
						latitude_degreefen=latitude_degreefen*10+p[i]-48;//γ�ȣ���
					}
					else if(dot_log==2)
					{
						latitude_minute=latitude_minute*10+p[i]-48;//γ�ȣ���
					}
				}
			}
		}
		delay_ms(5);
	}
}


