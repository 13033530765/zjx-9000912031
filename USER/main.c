#include "led.h"
#include "delay.h"
#include "key.h"
#include "sys.h"
#include "usart.h"
#include "uart2.h"
#include "RingBuffer.h"
#include "atk_m750.h"
#include "string.h"
#include "gps.h"
#include "stm32f10x.h"
#include "exti.h" 
#include "ultra.h"
#include "mpu6050.h"//MPU6050驱动库
#include "inv_mpu.h"//陀螺仪驱动库
#include "Timer.h"//定时器驱动库
//#include "stdio.h"//标准输入输出库
//#include "string.h"//字符串库
//#include "stdlib.h"//常用的系统函数库
//#include "sys.h"//系统中断分组库
//#include "delay.h"//延时函数库
//#include "usart.h"//串口设置库
//#include "led.h"//LED驱动库
//#include "Timer.h"//定时器驱动库
//#include "mpu6050.h"//MPU6050驱动库
//#include "inv_mpu.h"//陀螺仪驱动库
//#include "ultra.h"

float pitch,roll,yaw; 		//欧拉角:俯仰角，偏航角，滚转角
//short aacx,aacy,aacz;		//加速度传感器原始数据  angular acceleration
//short gyrox,gyroy,gyroz;	//陀螺仪原始数据  gyroscope
//short temp;					//温度

int main(void);				//主函数
void SYS_Init(void);		//系统初始化总函数



#define DTU_TEST_DATA "ALIENTEK ATK-IDM750C TEST"

#define DTU_NETDATA_RX_BUF (1024)

static uint32_t dtu_rxlen = 0;
static uint8_t dtu_rxbuf[DTU_NETDATA_RX_BUF];


RingBuffer *p_uart2_rxbuf;

int main(void)
{
    int ret;
    uint32_t timeout = 0;
    uint8_t buf;
    uint8_t key;
			double distance = 0; //????

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); /*设置NVIC中断分组2:2位抢占优先级，2位响应优先级*/
//	uart_init(115200);                              /*串口1初始化，波特率为115200*/

	
//	EXTIX_Init();
    delay_init();                                   /*延时函数初始化*/
    LED_Init();                                     /*LED端口初始化*/
	KEY_Init();
	MPU_Init();	                                     //初始化MPU6050
	UltrasonicWave_Init();//???
	mpu_dmp_init();
//	delay_ms(8000);
//	printf("ok111");
//	TIM3_Int_Init(499,7199);					//50ms，中断时间不可过慢，如无数据，请提高速度
////	delay_ms(8000);
//printf("ok");

    my_mem_init(SRAMIN);                            /*初始化内存池*/

    p_uart2_rxbuf = RingBuffer_Malloc(1024);        /*从内存池中分配1K的内存给串口3接收DTU数据*/
uart_init(115200); 
    uart2_init(115200);

    /*
        *  @arg        DTU_WORKMODE_NET,         网络透传模式
        *  @arg        DTU_WORKMODE_HTTP,        http透传模式
        *  @arg        DTU_WORKMODE_MQTT,        mqtt透传模式
        *  @arg        DTU_WORKMODE_ALIYUN,      阿里云透传模式
        *  @arg        DTU_WORKMODE_ONENET,      OneNET透传模式
        *  @arg        DTU_WORKMODE_BAIDUYUN,    百度云透传模式

        说明：每个模式都需要进行参数配置，请在atk_m750.c文件下找到以下定义进行相应的参数修改方可使用，默认参数不保证能正常传输数据！！！
        dtu_net_param_info/dtu_http_param_info/dtu_mqtt_param_info/dtu_aliyun_param_info/dtu_onenet_param_info/dtu_baiduyun_param_info
    */
    printf("Wait for Cat1 DTU to start, wait 10s.... \r\n");
    {
        while( timeout <= 10 )   /* 等待Cat1 DTU启动，需要等待5-6s才能启动 */
        {  
            ret = dtu_config_init(DTU_WORKMODE_NET);    /*初始化DTU工作参数*/
            if( ret == 0 )
                break;
            timeout++;
            delay_ms(1000);
        }

        while( timeout > 10 )   /* 超时 */
        {
            printf("**************************************************************************\r\n");
            printf("ATK-DTU Init Fail ...\r\n");
            printf("请按照以下步骤进行检查:\r\n");
            printf("1.使用电脑上位机配置软件检查DTU能否单独正常工作\r\n");
            printf("2.检查DTU串口参数与STM32通讯的串口参数是否一致\r\n");
            printf("3.检查DTU与STM32串口的接线是否正确\r\n");
            printf("4.检查DTU供电是否正常，DTU推荐使用12V/1A电源供电，不要使用USB的5V给模块供电！！\r\n");
            printf("**************************************************************************\r\n\r\n");
            delay_ms(3000);
        }
    }
    printf("Cat1 DTU Init Success \r\n");

    dtu_rxlen = 0;
    RingBuffer_Reset(p_uart2_rxbuf);

    /*  
        DTU进入透传状态后，就可以把它当成普通串口使用，必须确保：1.硬件连接完好 2.串口参数与DTU的保持一致
        注意：DTU每次上电需要一定的时间，在等待连接过程中，MCU可以向DTU发送数据并缓存在DTU中，等到与服务器连接上，DTU会自动将数据缓存数据全部转发到服务器上。
    */
//    while (1)
//    {
//        if (RingBuffer_Len(p_uart2_rxbuf) > 0)          /*接收到DTU传送过来的服务器数据*/
//        {
//            RingBuffer_Out(p_uart2_rxbuf, &buf, 1);

//            dtu_rxbuf[dtu_rxlen++] = buf;
//            dtu_get_urc_info(buf);                      /*解析DTU上报的URC信息*/

//            if (dtu_rxlen >= DTU_NETDATA_RX_BUF)        /*接收缓存溢出*/
//            {
//                usart1_send_data(dtu_rxbuf, dtu_rxlen); /*接收到从DTU传过来的网络数据，转发到调试串口1输出*/
//                dtu_rxlen = 0;
//            }
//        }
//        else
//        {
//            if (dtu_rxlen > 0)
//            {
//                usart1_send_data(dtu_rxbuf, dtu_rxlen); /*接收到从DTU传过来的网络数据，转发到调试串口1输出*/
//                dtu_rxlen = 0;
//            }

//            LED0 = !LED0;
//            delay_ms(100);
//        }

//        key = KEY_Scan(0);
//        if(key == KEY0_PRES)
//        {
//            /*如果服务器格式对数据有要求，请修改对应的数据格式，这里只限于例程测试使用*/
//            send_data_to_dtu((uint8_t *)DTU_TEST_DATA, strlen(DTU_TEST_DATA));
//      
	
	delay_ms(5000);
	clrStruct();
////		TIM3_Int_Init(499,7199);					//50ms，中断时间不可过慢，如无数据，请提高速度
//dtu_base_station_orientation();
//		delay_ms(1000);
//		printf("ok");
//		dtu_base_station_orientation_data_send();
while(1)
{
	distance = UltrasonicWave_Measure();
						printf("distance:%f \r\n",distance);
	if(!KEY_Scan(1))
	{
			LED0=!LED0;
parseGpsBuffer();
		delay_ms(1000);
//		printf("ok2");
//					printGpsBuffer();

		sendGpsBuffer();
		delay_ms(5000);
		CLR_Buf();
			dtu_base_station_orientation();
		delay_ms(1000);
//		printf("ok2  ");
		dtu_base_station_orientation_data_send();
	}
		else if(mpu_dmp_get_data(&pitch,&roll,&yaw)==0)//dmp处理得到数据，对返回值进行判断
			{ 
//				temp=MPU_Get_Temperature();	                //得到温度值
//				MPU_Get_Accelerometer(&aacx,&aacy,&aacz);	//得到加速度传感器数据
//				MPU_Get_Gyroscope(&gyrox,&gyroy,&gyroz);	//得到陀螺仪数据
				
//				if(temp<0)								//对数据正负判断，判断为负时
//				{
//					temp=-temp;							//对负数据取反
//				}
//				else                                    //判断为正时
//				{
//				}
//				printf(" temp:%d.%d ",temp/100,temp%10); //通过串口1输出温度
				
//				temp=pitch*10;							 //赋temp为pitch
				if(pitch<0)								//对数据正负判断，判断为负时
				{
					pitch=-pitch;						    //对负数据取反		
				}
				else                                    //判断为正时 
				{
				}
//				double JIAO=0;
//				JIAO=temp/10;
//				temp=(temp%10)*0.1;
//				JIAO=JIAO+temp;
					printf("pitch:%f \r\n",pitch);
				
			if (pitch>5)
				{
//					 LED0 = 0; 
					 VIB2=1; //震动模块
					}
			else 
			{
//				  LED0 = 1;
				  VIB2=0;
			}
//			double distance = 0; //????
}
//			distance = UltrasonicWave_Measure();
//		    delay_ms(60);//??????? 60ms??, ????????????????
			else if (distance<60&&distance>10)
				{
//					 LED0 = 0; 
					 VIB1=1; //震动模块
					}
			else if(distance>60||distance<10)
			{
//				  LED0 = 1;
				  VIB1=0;
			}
//			clrStruct();

//				parseGpsBuffer();
//					printf("  ok  ");
//		delay_ms(1000);
//			sendGpsBuffer();
////		ret1=sendGpsBuffer();
//					delay_ms(1000);
//					printf("ok1  ");

//	while(1)
//	{
//		if(!KEY_Scan(1))
//		{
//			int ret1=0;
//			int ret2=0;
//			LED0=!LED0;
////		dtu_base_station_orientation();
////		delay_ms(1000);
////		printf("ok");
////		ret1=dtu_base_station_orientation_data_send();
//			clrStruct();

//				parseGpsBuffer();
//					printf("  ok  ");
//		delay_ms(1000);
//			printGpsBuffer();
//			sendGpsBuffer();
////		ret1=sendGpsBuffer();
//					delay_ms(1000);
//					printf("ok1  ");
//						clrStruct();

////				dtu_base_station_orientation();
////		delay_ms(1000)KK;
////		printf("ok2  ");
////		dtu_base_station_orientation_data_send();
////					delay_ms(1000);
////		printf("ok3  ");
////			if(ret1==1||ret2==1)
////			{
////				VIB1=1;
////				VIB2=1;
////				delay_ms(5000);
////				VIB1=0;
////				VIB2=0;
////			}
//		delay_ms(1000);
//		}
//		if(mpu_dmp_get_data(&pitch,&roll,&yaw)==0)//dmp处理得到数据，对返回值进行判断
//			{ 
////				temp=MPU_Get_Temperature();	                //得到温度值
////				MPU_Get_Accelerometer(&aacx,&aacy,&aacz);	//得到加速度传感器数据
////				MPU_Get_Gyroscope(&gyrox,&gyroy,&gyroz);	//得到陀螺仪数据
//				
////				if(temp<0)								//对数据正负判断，判断为负时
////				{
////					temp=-temp;							//对负数据取反
////				}
////				else                                    //判断为正时
////				{
////				}
////				printf(" temp:%d.%d ",temp/100,temp%10); //通过串口1输出温度
//				
////				temp=pitch*10;							 //赋temp为pitch
//				if(pitch<0)								//对数据正负判断，判断为负时
//				{
//					pitch=-pitch;						    //对负数据取反		
//				}
//				else                                    //判断为正时 
//				{
//				}
////				double JIAO=0;
////				JIAO=temp/10;
////				temp=(temp%10)*0.1;
////				JIAO=JIAO+temp;
//			if (pitch>15)
//				{
////					 LED0 = 0; 
//					 VIB2=1; //震动模块
//					}
//			else 
//			{
////				  LED0 = 1;
//				  VIB2=0;
//			}
////				printf(" pitch:%f ",pitch); //通过串口1输出pitch	
//				
////				temp=roll*10;                            //赋temp为roll
////				if(temp<0)								//对数据正负判断，判断为负时
////				{
////					temp=-temp;						    //对负数据取反	
////				}
////				else                                    //判断为正时
////				{
////				}
////				printf(" roll:%d.%d ",temp/10,temp%10);//通过串口1输出roll
////				
////				temp=yaw*10;                           //赋temp为yaw
////				if(temp<0)								//对数据正负判断，判断为负时
////				{
////					temp=-temp;						    //对负数据取反
////				}
////				else                                    //判断为正时
////				{
////				}
////				printf(" yaw:%d.%d \r\n",temp/10,temp%10);//通过串口1输出yaw	
//				
////				LED0=!LED0;								 			//LED闪烁
//			}

//		    double distance = UltrasonicWave_Measure(); //????
//		    delay_ms(60);//??????? 60ms??, ????????????????
//			if (distance<20)
//				{
////					 LED0 = 0; 
//					 VIB1=1; //震动模块
//					}
//			else 
//			{
////				  LED0 = 1;
//				  VIB1=0;
//			}
////		    printf("distance:%5.2f \r\n",distance);//?????
//		
//		
////		dtu_base_station_orientation();
////		dtu_base_station_orientation_data_processing();
////		delay_ms(1000);
////		printf("ok1");
////		parseGpsBuffer();
////		printGpsBuffer();
////		delay_ms(1000);
////		printf("ok2");
//	
//		}
    
    }
	}
