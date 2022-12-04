#include "atk_m750.h"

unsigned char  MQTT_RxDataBuf[R_NUM][BUFF_UNIT];            //数据的接收缓冲区,所有服务器发来的数据，存放在该缓冲区,缓冲区第一个字节存放数据长度
unsigned char *MQTT_RxDataInPtr;                            //指向接收缓冲区存放数据的位置
unsigned char *MQTT_RxDataOutPtr;                           //指向接收缓冲区读取数据的位置
unsigned char *MQTT_RxDataEndPtr;                           //指向接收缓冲区结束的位置

unsigned char  MQTT_TxDataBuf[T_NUM][BUFF_UNIT];            //数据的发送缓冲区,所有发往服务器的数据，存放在该缓冲区,缓冲区第一个字节存放数据长度
unsigned char *MQTT_TxDataInPtr;                            //指向发送缓冲区存放数据的位置
unsigned char *MQTT_TxDataOutPtr;                           //指向发送缓冲区读取数据的位置
unsigned char *MQTT_TxDataEndPtr;                           //指向发送缓冲区结束的位置

unsigned char  MQTT_CMDBuf[C_NUM][BUFF_UNIT];               //命令数据的接收缓冲区
unsigned char *MQTT_CMDInPtr;                               //指向命令缓冲区存放数据的位置
unsigned char *MQTT_CMDOutPtr;                              //指向命令缓冲区读取数据的位置
unsigned char *MQTT_CMDEndPtr;                              //指向命令缓冲区结束的位置

int   Fixed_len;                       					     //固定报头长度
int   Variable_len;                     					 //可变报头长度
int   Payload_len;                       					 //有效负荷长度
unsigned char  temp_buff[BUFF_UNIT];						 //临时缓冲区，构建报文用

extern RingBuffer *p_uart2_rxbuf;
extern u8 dtu_state;
uint8_t dtu_rxcmdbuf[DTU_RX_CMD_BUF_SIZE]; /*处理DTU相关数据缓存*/
uint32_t dtu_rxlen = 0;
uint8_t dtu_rxbuf[DTU_NETDATA_RX_BUF];

/*测试变量*/
u8 current;
u8 longitude_degreefen=0,latitude_degreefen=0;//经度，度。纬度，度
u32 longitude_minute=0,latitude_minute=0;//经度，分。纬度，分
extern u8 PitchVPT,RollVPT;
extern float Pitch,Roll;
char p[128],identifier[128];
extern u8 MPU6050_Pitch_y,MPU6050_Roll_y;
extern u8 set_PitchVPT_log,set_RollVPT_log;
char cell_phone_number[11]={"15388908342"};




/**
 * @brief       发送数据到DTU
 * 
 * @param       data:   需要发送数据的缓存地址
 * @param       size:   发送数据大小
 * 
 * @return      无
 * 
*/
void send_data_to_dtu(uint8_t *data, uint32_t size)
{
    usart2_send_data(data, size);
}

/**
 * @brief       发送命令到DTU并进行数据校验
 * 
 * @param       cmd     :   需要发送的AT指令
 * @param       ask     :   需要校验的应答数据
 * @param       timeout :   AT指令校验超时时间，单位：100ms
 * 
 * @return      1  :   校验ask数据成功
 *              0  :   DTU返回OK
 *             -1  :   DTU返回ERROR
 *             -2  :   发送AT指令校验超时
 */
static int send_cmd_to_dtu(char *cmd, char *ask, uint32_t timeout)
{
    uint32_t rx_len = 0;

    /*初始化缓存数据*/
    memset(dtu_rxcmdbuf, 0, DTU_RX_CMD_BUF_SIZE);
    RingBuffer_Reset(p_uart2_rxbuf);

    /*发送AT指令到DTU*/
    send_data_to_dtu((uint8_t *)cmd, strlen(cmd));

    /*等待DTU应答AT指令结果*/
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
            RingBuffer_Out(p_uart2_rxbuf, &dtu_rxcmdbuf[rx_len++], 1); //从串口缓存中读一个字节

            if (rx_len >= DTU_RX_CMD_BUF_SIZE) /*接收应答数据超长，返回ERROR*/
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
 * @brief       DTU进入配置状态
 * 
 * @param       无
 * 
 * @return      0  :    成功进入配置状态
 *             -1  :    进入配置状态失败
 */
int dtu_enter_configmode(void)
{
    int res;

    /* 1.发送+++准备进入配置状态 */
    res = send_cmd_to_dtu("+++", "atk", 5);
    if (res == -1) /*返回ERRRO表示DTU已经处于配置状态*/
    {	
			dtu_state=0;
        return 0;
    }

    /* 2.发送atk确认进入配置状态 */
    res = send_cmd_to_dtu("atk", "OK", 5);
    if (res == -2)
    {
        return -1;
    }
		dtu_state=0;
    return 0;
}

/**
 * @brief       DTU进入透传状态
 * 
 * @param       无
 * 
 * @return      0  :    成功进入透传状态
 *             -1  :    进入透传状态失败
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
 * @brief       DTU自动上报URC信息处理函数:处理+ATK ERROR信息
 * 
 * @param       data    :   接收到DTU的URC数据缓存
 * @param       len     :   URC数据长度
 * 
 * @return      无
 */
static void dtu_urc_atk_error(const char *data, uint32_t len)
{
//    printf("\r\nURC :   dtu_urc_atk_error\r\n");
}

/**
 * @brief       DTU自动上报URC信息处理函数:处理Please check SIM Card信息
 * 
 * @param       data    :   接收到DTU的URC数据缓存
 * @param       len     :   URC数据长度
 * 
 * @return      无
 */
static void dtu_urc_error_sim(const char *data, uint32_t len)
{
//    printf("\r\nURC :   dtu_urc_error_sim\r\n");
}

/**
 * @brief       DTU自动上报URC信息处理函数:处理Please check GPRS信息
 * 
 * @param       data    :   接收到DTU的URC数据缓存
 * @param       len     :   URC数据长度
 * 
 * @return      无
 */
static void dtu_urc_error_gprs(const char *data, uint32_t len)
{
//    printf("\r\nURC :   dtu_urc_error_gprs\r\n");
}

/**
 * @brief       DTU自动上报URC信息处理函数:处理Please check CSQ信息
 * 
 * @param       data    :   接收到DTU的URC数据缓存
 * @param       len     :   URC数据长度
 * 
 * @return      无
 */
static void dtu_urc_error_csq(const char *data, uint32_t len)
{
//    printf("\r\nURC :   dtu_urc_error_csq\r\n");
}

/**
 * @brief       DTU自动上报URC信息处理函数:处理Please check MQTT Parameter信息
 * 
 * @param       data    :   接收到DTU的URC数据缓存
 * @param       len     :   URC数据长度
 * 
 * @return      无
 */
static void dtu_urc_error_mqtt(const char *data, uint32_t len)
{
//    printf("\r\nURC :   dtu_urc_error_mqtt\r\n");
}

typedef struct
{
    const char *urc_info;                         /*DTU自动上报的URC信息*/
    void (*func)(const char *data, uint32_t len); /*回调处理函数*/
} _dtu_urc_st;

#define DTU_ATK_M750_URC_SIZE 5
static _dtu_urc_st DTU_ATK_M750_URC[DTU_ATK_M750_URC_SIZE] =
    {
       
        {"+ATK ERROR:",                         dtu_urc_atk_error},         /*DTU存在问题，需要联系技术支持进行确认*/
        {"Please check SIM Card !!!\r\n",       dtu_urc_error_sim},         /*DTU未检测到手机卡,请检查手机卡是否正确插入*/
        {"Please check GPRS !!!\r\n",           dtu_urc_error_gprs},        /*请检查SIM卡是否欠费*/
        {"Please check CSQ !!!\r\n",            dtu_urc_error_csq},         /*请检查天线是否正确接入，并确保天线位置的正确性*/
        {"Please check MQTT Parameter !!!\r\n", dtu_urc_error_mqtt},        /*MQTT参数有误*/
};

/**
 * @brief       处理DTU主动上报的URC信息数据，注意：串口每接收一个字节数据，都需要通过函数入口传输进来
 * 
 * @param       ch    :   串口接收的一个字节数据
 * 
 * @return      无
 */
void dtu_get_urc_info(uint8_t ch)
{
    static uint8_t ch_last = 0;
    static uint32_t rx_len = 0;
    int i;

    /*接收DTU数据*/
    dtu_rxcmdbuf[rx_len++] = ch;
    if (rx_len >= DTU_RX_CMD_BUF_SIZE)
    { /*超长处理*/
        ch_last = 0;
        rx_len = 0;
        memset(dtu_rxcmdbuf, 0, DTU_RX_CMD_BUF_SIZE);
    }

    /*处理DTU的URC数据*/
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

    /*1.选择工作模式为：网络透传模式*/
    {5, "AT+WORK\r\n",      "AT+WORK=\"NET\"\r\n"},

    /*2.配置网络透传模式的工作参数*/
    {5, "AT+LINK1EN\r\n",   "AT+LINK1EN=\"ON\"\r\n"},
    {5, "AT+LINK1\r\n",     "AT+LINK1=\"TCP\",\"cloud.alientek.com\",\"59666\"\r\n"},
    {5, "AT+LINK1MD\r\n",   "AT+LINK1MD=\"LONG\"\r\n"},
    {5, "AT+LINK1TM\r\n",   "AT+LINK1TM=\"5\"\r\n"},

    {5, "AT+LINK2EN\r\n",   "AT+LINK2EN=\"OFF\"\r\n"},
    {5, "AT+LINK3EN\r\n",   "AT+LINK3EN=\"OFF\"\r\n"},
    {5, "AT+LINK4EN\r\n",   "AT+LINK4EN=\"OFF\"\r\n"},

    /*3.配置原子云功能：默认打开*/
    {5, "AT+SVREN\r\n",     "AT+SVREN=\"ON\"\r\n"},
    {5, "AT+SVRNUM\r\n",    "AT+SVRNUM=\"12345678901234567890\"\r\n"},
    {5, "AT+SVRKEY\r\n",    "AT+SVRKEY=\"12345678\"\r\n"},

    /*4.配置心跳包功能：默认打开         注意：强烈建议开启心跳包功能！！！*/
    {5, "AT+HRTEN\r\n",     "AT+HRTEN=\"ON\"\r\n"},
    {5, "AT+HRTDT\r\n",     "AT+HRTDT=\"414C49454E54454B2D4852544454\"\r\n"},
    {5, "AT+HRTTM\r\n",     "AT+HRTTM=\"120\"\r\n"},

    /*5.配置注册包功能：默认关闭 */
    {5, "AT+REGEN\r\n",     "AT+REGEN=\"OFF\"\r\n"},
    {5, "AT+REGDT\r\n",     "AT+REGDT=\"414C49454E54454B2D5245474454\"\r\n"},
    {5, "AT+REGMD\r\n",     "AT+REGMD=\"LINK\"\r\n"},
    {5, "AT+REGTP\r\n",     "AT+REGTP=\"IMEI\"\r\n"},

    /*6.其他参数功能配置*/
};

static const _dtu_atcmd_st dtu_http_param_info[] = {

    /*1.选择工作模式为：HTTP透传模式*/
    {5, "AT+WORK\r\n",      "AT+WORK=\"HTTP\"\r\n"},

    /*2.配置HTTP透传模式的工作参数*/
    {5, "AT+HTTPMD\r\n",    "AT+HTTPMD=\"GET\"\r\n"},
    {5, "AT+HTTPURL\r\n",   "AT+HTTPURL=\"https://cloud.alientek.com/testfordtu?data=\"\r\n"},
    {5, "AT+HTTPTM\r\n",    "AT+HTTPTM=\"10\"\r\n"},
    {5, "AT+HTTPHD\r\n",    "AT+HTTPHD=\"Connection:close\"\r\n"},

    /*3.其他参数功能配置*/
};

static const _dtu_atcmd_st dtu_mqtt_param_info[] = {

    /*1.选择工作模式为：MQTT透传模式*/
    {5, "AT+WORK\r\n",      "AT+WORK=\"MQTT\"\r\n"},

    /*2.配置MQTT透传模式的工作参数*/
    {5, "AT+MQTTCD\r\n",    "AT+MQTTCD=\"alientek\"\r\n"},
    {5, "AT+MQTTUN\r\n",    "AT+MQTTUN=\"admin\"\r\n"},
    {5, "AT+MQTTPW\r\n",    "AT+MQTTPW=\"password\"\r\n"},
    {5, "AT+MQTTIP\r\n",    "AT+MQTTIP=\"cloud.alientek.com\",\"1883\"\r\n"},
    {5, "AT+MQTTSUB\r\n",   "AT+MQTTSUB=\"atk/sub\"\r\n"},
    {5, "AT+MQTTPUB\r\n",   "AT+MQTTPUB=\"atk/pub\"\r\n"},
    {5, "AT+MQTTCON\r\n",   "AT+MQTTCON=\"0\",\"0\",\"1\",\"300\"\r\n"},

    /*3.其他参数功能配置*/
};

static const _dtu_atcmd_st dtu_aliyun_param_info[] = {

    /*1.选择工作模式为：阿里云透传模式*/
    {5, "AT+WORK\r\n",      "AT+WORK=\"ALIYUN\"\r\n"},

    /*2.配置MQTT透传模式的工作参数*/
    {5, "AT+ALIPK\r\n",     "AT+ALIPK=\"a1dWcp4lRBu\"\r\n"},
    {5, "AT+ALIDS\r\n",     "AT+ALIDS=\"dee0ce25d3b6542b47ce09c7500af3ed\"\r\n"},
    {5, "AT+ALIDN\r\n",     "AT+ALIDN=\"a1dWcp4lRBu\"\r\n"},
    {5, "AT+ALIRI\r\n",     "AT+ALIRI=\"cn-shanghai\"\r\n"},
    {5, "AT+ALISUB\r\n",    "AT+ALISUB=\"/sys/a1dWcp4lRBu/a1dWcp4lRBu/thing/service/property/set\"\r\n"},
    {5, "AT+ALIPUB\r\n",    "AT+ALIPUB=\"/sys/a1dWcp4lRBu/a1dWcp4lRBu/thing/event/property/post\"\r\n"},
    {5, "AT+ALICON\r\n",    "AT+ALICON=\"0\",\"0\",\"1\",\"300\"\r\n"},

    /*3.其他参数功能配置*/
};

static const _dtu_atcmd_st dtu_onenet_param_info[] = {

    /*1.选择工作模式为：OneNET透传模式*/
    {5, "AT+WORK\r\n",      "AT+WORK=\"ONENET\"\r\n"},

    /*2.配置OneNET透传模式的工作参数*/
    {5, "AT+ONEDI\r\n",     "AT+ONEDI=\"12345\"\r\n"},
    {5, "AT+ONEPI\r\n",     "AT+ONEPI=\"1234567890\"\r\n"},
    {5, "AT+ONEAI\r\n",     "AT+ONEAI=\"12345678901234567890\"\r\n"},
    {5, "AT+ONEIP\r\n",     "AT+ONEIP=\"mqtt.hecloud.com\",\"6002\"\r\n"},
    {5, "AT+ONECON\r\n",    "AT+ONECON=\"1\",\"0\",\"0\",\"1\",\"300\"\r\n"},

    /*3.其他参数功能配置*/
};

static const _dtu_atcmd_st dtu_baiduyun_param_info[] = {

    /*1.选择工作模式为：百度云透传模式*/
    {5, "AT+WORK\r\n",      "AT+WORK=\"BAIDUYUN\"\r\n"},

    /*2.配置百度云透传模式的工作参数*/
    {5, "AT+BAIEP\r\n",     "AT+BAIEP=\"alientek\"\r\n"},
    {5, "AT+BAINM\r\n",     "AT+BAINM=\"name\"\r\n"},
    {5, "AT+BAIKEY\r\n",    "AT+BAIKEY=\"key\"\r\n"},
    {5, "AT+BAIRI\r\n",     "AT+BAIRI=\"gz\"\r\n"},
    {5, "AT+BAISUB\r\n",    "AT+BAISUB=\"sub\"\r\n"},
    {5, "AT+BAIPUB\r\n",    "AT+BAIPUB=\"pub\"\r\n"},
    {5, "AT+BAICON\r\n",    "AT+BAICON=\"0\",\"0\",\"0\",\"1\",\"300\"\r\n"},

    /*3.其他参数功能配置*/
};

/**
 * @brief       配置DTU工作参数
 * 
 * @param       work_param      :   工作模式相关AT指令参数
 * @param       num             :   需要配置的AT指令参数数量
 * 
 * @return      0  :    所有参数配置成功
 *              n  :    第几个参数配置失败：1-n
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

        if (res == 1) /*如果DTU内部参数和需要配置的参数一致，不需要重复去配置*/
        {
            continue;
        }
        else /*DTU内部参数与配置参数不一致，需要更新DTU内部参数*/
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
 * @brief       初始化DTU的工作状态
 * 
 * @param       work_mode   :   DTU工作模式
 *  @arg        DTU_WORKMODE_NET,       0,  网络透传模式
 *  @arg        DTU_WORKMODE_HTTP,      1,  http透传模式
 *  @arg        DTU_WORKMODE_MQTT,      2,  mqtt透传模式
 *  @arg        DTU_WORKMODE_ALIYUN,    3,  阿里云透传模式
 *  @arg        DTU_WORKMODE_ONENET,    4,  OneNET透传模式
 *  @arg        DTU_WORKMODE_BAIDUYUN,  5,  百度云透传模式
 * 
 * @return      0   :   初始化成功
 *             -1   :   进入配状态失败
 *             -2   :   DTU工作参数配置失败
 *             -3   ：  DTU进入透传状态失败
 */
int dtu_config_init(_dtu_work_mode_eu work_mode)
{
    int res;

    /*1.DTU进入配置状态*/
    res = dtu_enter_configmode();
    if ( res != 0 )
    {
        return -1;
    }

    /*2.配置DTU的工作参数*/
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

    /*3.DTU进入透传状态*/
    res = dtu_enter_transfermode();
    if( res != 0 )
    {
        return -3;
    }

    return 0;
}

/*----------------------------------------------------------*/
/*函数名：处理发送缓冲区                                    */
/*参  数：data：数据                                        */
/*参  数：size：数据长度                                    */
/*返回值：无                                                */
/*----------------------------------------------------------*/
void TxDataBuf_Deal(unsigned char *data, int size)
{
	memcpy(&MQTT_TxDataInPtr[2],data,size);      //拷贝数据到发送缓冲区	
	MQTT_TxDataInPtr[0] = size/256;              //记录数据长度
	MQTT_TxDataInPtr[1] = size%256;              //记录数据长度
	MQTT_TxDataInPtr+=BUFF_UNIT;                 //指针下移
	if(MQTT_TxDataInPtr==MQTT_TxDataEndPtr)      //如果指针到缓冲区尾部了
		MQTT_TxDataInPtr = MQTT_TxDataBuf[0];    //指针归位到缓冲区开头
}


/*----------------------------------------------------------*/
/*函数名：等级0 发布消息报文                                */
/*参  数：topic_name：topic名称                             */
/*参  数：data：数据                                        */
/*参  数：data_len：数据长度                                */
/*返回值：无                                                */
/*----------------------------------------------------------*/
void MQTT_PublishQs0(char *topic, char *data, int data_len)
{	
	int temp,Remaining_len;
	
	Fixed_len = 1;                              //固定报头长度暂时先等于：1字节
	Variable_len = 2 + strlen(topic);           //可变报头长度：2字节(topic长度)+ topic字符串的长度
	Payload_len = data_len;                     //有效负荷长度：就是data_len
	Remaining_len = Variable_len + Payload_len; //剩余长度=可变报头长度+负载长度
	
	temp_buff[0]=0x30;                       //固定报头第1个字节 ：固定0x30   	
	do{                                      //循环处理固定报头中的剩余长度字节，字节量根据剩余字节的真实长度变化
		temp = Remaining_len%128;            //剩余长度取余128
		Remaining_len = Remaining_len/128;   //剩余长度取整128
		if(Remaining_len>0)               	
			temp |= 0x80;                    //按协议要求位7置位          
		temp_buff[Fixed_len] = temp;         //剩余长度字节记录一个数据
		Fixed_len++;	                     //固定报头总长度+1    
	}while(Remaining_len>0);                 //如果Remaining_len>0的话，再次进入循环
		             
	temp_buff[Fixed_len+0]=strlen(topic)/256;                      //可变报头第1个字节     ：topic长度高字节
	temp_buff[Fixed_len+1]=strlen(topic)%256;		               //可变报头第2个字节     ：topic长度低字节
	memcpy(&temp_buff[Fixed_len+2],topic,strlen(topic));           //可变报头第3个字节开始 ：拷贝topic字符串	
	memcpy(&temp_buff[Fixed_len+2+strlen(topic)],data,data_len);   //有效负荷：拷贝data数据
	
	TxDataBuf_Deal(temp_buff, Fixed_len + Variable_len + Payload_len);  //加入发送数据缓冲区
}


void AliIoT_processing_data(void)
{
	int p_1;
	u16 numerical=0;
	uint8_t buf;
	char temp[256];
	if (RingBuffer_Len(p_uart2_rxbuf) > 0)          /*接收到DTU传送过来的服务器数据*/
  {
		RingBuffer_Out(p_uart2_rxbuf, &buf, 1);
    dtu_rxbuf[dtu_rxlen++] = buf;
    dtu_get_urc_info(buf);                      /*解析DTU上报的URC信息*/
    if (dtu_rxlen >= DTU_NETDATA_RX_BUF)        /*接收缓存溢出*/
    {
//      usart1_send_data(dtu_rxbuf, dtu_rxlen); /*接收到从DTU传过来的网络数据，转发到调试串口1输出*/
      dtu_rxlen = 0;
    }
  }
  else
  {
		if (dtu_rxlen>0)
    {
			if(strstr((char *)dtu_rxcmdbuf,"\"params\":{\"PitchVPT\""))//strstr((char *)dtu_rxcmdbuf, ask)
			{//如果搜索到"params":{"LightSwitch":1}说明服务器下发打开开关1	
				sprintf(p,"%s",strstr((char *)dtu_rxbuf,"\"params\":{\"PitchVPT\""));
				sprintf(identifier,"%s","PitchVPT");
				p_1=13;
				p_1=p_1+8;//current的长度为7
				while(p[p_1]!='}')
				{
					numerical=numerical*10+p[p_1]-48;
					p_1++;
				}
				PitchVPT=numerical;
				set_PitchVPT_log=1;
				LCD_Fill(115+5,MPU6050_Pitch_y,115+5+43,MPU6050_Pitch_y+14,WHITE);
				LCD_ShowFloat(115+7,MPU6050_Pitch_y,16,PitchVPT,5,0);
				sprintf(temp,"{\"method\":\"thing.event.property.post\",\"id\":\"203302322\",\"params\":{\"PitchVPT\":%d},\"version\":\"1.0.0\"}",PitchVPT);  //构建jw01数据回复
				usart2_send_data((u8 *)temp,strlen(temp));
//				AlibabaCloudIssuedTheLogo=1;
				//判断开关状态，并发布给服务器  
			}
			else if(strstr((char *)dtu_rxbuf,"\"params\":{\"RollVPT\""))
			{
				sprintf(p,"%s",strstr((char *)dtu_rxbuf,"\"params\":{\"RollVPT\""));
				sprintf(identifier,"%s","RollVPT");
				p_1=13;
				p_1=p_1+7;//current的长度为7
				while(p[p_1]!='}')
				{
					numerical=numerical*10+p[p_1]-48;
					p_1++;
				}
				RollVPT=numerical;
				set_RollVPT_log=1;
				LCD_Fill(115+5,MPU6050_Roll_y,115+5+43,MPU6050_Roll_y+14,WHITE);
				LCD_ShowFloat(115+7,MPU6050_Roll_y,16,RollVPT,5,0);
				sprintf(temp,"{\"method\":\"thing.event.property.post\",\"id\":\"203302322\",\"params\":{\"RollVPT\":%d},\"version\":\"1.0.0\"}",RollVPT);  //构建jw01数据回复
				usart2_send_data((u8 *)temp,strlen(temp));
			}
			delay_ms(10);
//      usart1_send_data(dtu_rxbuf, dtu_rxlen); /*接收到从DTU传过来的网络数据，转发到调试串口1输出*/
      dtu_rxlen = 0;
    }
     delay_ms(10);
  }

}

int dtu_send_sms(char *phone, char *sms_msg)//发短信
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
		printf("发送成功");
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
int dtu_base_station_orientation(void)//基站定位
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
			if(RingBuffer_Len(p_uart2_rxbuf) > 0)          /*接收到DTU传送过来的服务器数据*/
			{
				RingBuffer_Out(p_uart2_rxbuf, &buf, 1);
				dtu_rxbuf[dtu_rxlen++] = buf;
				if (dtu_rxlen >= DTU_NETDATA_RX_BUF)        /*接收缓存溢出*/
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
	u8 dot_log=0,comma_log=0;//点标志。逗号标志
	if(strstr((char *)dtu_rxbuf,"+LOCATION"))//+LOCATION:120.168640,35.969391
		//+LOCATION:120.168640,35.969391
	{
		delay_ms(10);
		longitude_degreefen=0;latitude_degreefen=0;//经度，度。纬度，度
		longitude_minute=0;latitude_minute=0;//经度，分。纬度，分
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
						longitude_degreefen=longitude_degreefen*10+p[i]-48;//经度，度
					}
					else if(dot_log==1)
					{
						longitude_minute=longitude_minute*10+p[i]-48;//经度，分
					}
				}
				else if(comma_log==1)
				{
					if(dot_log==1)
					{
						latitude_degreefen=latitude_degreefen*10+p[i]-48;//纬度，度
					}
					else if(dot_log==2)
					{
						latitude_minute=latitude_minute*10+p[i]-48;//纬度，分
					}
				}
			}
		}
		delay_ms(5);
	}
}



