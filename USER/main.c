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
#include "mpu6050.h"//MPU6050������
#include "inv_mpu.h"//������������
#include "Timer.h"//��ʱ��������
//#include "stdio.h"//��׼���������
//#include "string.h"//�ַ�����
//#include "stdlib.h"//���õ�ϵͳ������
//#include "sys.h"//ϵͳ�жϷ����
//#include "delay.h"//��ʱ������
//#include "usart.h"//�������ÿ�
//#include "led.h"//LED������
//#include "Timer.h"//��ʱ��������
//#include "mpu6050.h"//MPU6050������
//#include "inv_mpu.h"//������������
//#include "ultra.h"

float pitch,roll,yaw; 		//ŷ����:�����ǣ�ƫ���ǣ���ת��
//short aacx,aacy,aacz;		//���ٶȴ�����ԭʼ����  angular acceleration
//short gyrox,gyroy,gyroz;	//������ԭʼ����  gyroscope
//short temp;					//�¶�

int main(void);				//������
void SYS_Init(void);		//ϵͳ��ʼ���ܺ���



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

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); /*����NVIC�жϷ���2:2λ��ռ���ȼ���2λ��Ӧ���ȼ�*/
//	uart_init(115200);                              /*����1��ʼ����������Ϊ115200*/

	
//	EXTIX_Init();
    delay_init();                                   /*��ʱ������ʼ��*/
    LED_Init();                                     /*LED�˿ڳ�ʼ��*/
	KEY_Init();
	MPU_Init();	                                     //��ʼ��MPU6050
	UltrasonicWave_Init();//???
	mpu_dmp_init();
//	delay_ms(8000);
//	printf("ok111");
//	TIM3_Int_Init(499,7199);					//50ms���ж�ʱ�䲻�ɹ������������ݣ�������ٶ�
////	delay_ms(8000);
//printf("ok");

    my_mem_init(SRAMIN);                            /*��ʼ���ڴ��*/

    p_uart2_rxbuf = RingBuffer_Malloc(1024);        /*���ڴ���з���1K���ڴ������3����DTU����*/
uart_init(115200); 
    uart2_init(115200);

    /*
        *  @arg        DTU_WORKMODE_NET,         ����͸��ģʽ
        *  @arg        DTU_WORKMODE_HTTP,        http͸��ģʽ
        *  @arg        DTU_WORKMODE_MQTT,        mqtt͸��ģʽ
        *  @arg        DTU_WORKMODE_ALIYUN,      ������͸��ģʽ
        *  @arg        DTU_WORKMODE_ONENET,      OneNET͸��ģʽ
        *  @arg        DTU_WORKMODE_BAIDUYUN,    �ٶ���͸��ģʽ

        ˵����ÿ��ģʽ����Ҫ���в������ã�����atk_m750.c�ļ����ҵ����¶��������Ӧ�Ĳ����޸ķ���ʹ�ã�Ĭ�ϲ�������֤�������������ݣ�����
        dtu_net_param_info/dtu_http_param_info/dtu_mqtt_param_info/dtu_aliyun_param_info/dtu_onenet_param_info/dtu_baiduyun_param_info
    */
    printf("Wait for Cat1 DTU to start, wait 10s.... \r\n");
    {
        while( timeout <= 10 )   /* �ȴ�Cat1 DTU��������Ҫ�ȴ�5-6s�������� */
        {  
            ret = dtu_config_init(DTU_WORKMODE_NET);    /*��ʼ��DTU��������*/
            if( ret == 0 )
                break;
            timeout++;
            delay_ms(1000);
        }

        while( timeout > 10 )   /* ��ʱ */
        {
            printf("**************************************************************************\r\n");
            printf("ATK-DTU Init Fail ...\r\n");
            printf("�밴�����²�����м��:\r\n");
            printf("1.ʹ�õ�����λ������������DTU�ܷ񵥶���������\r\n");
            printf("2.���DTU���ڲ�����STM32ͨѶ�Ĵ��ڲ����Ƿ�һ��\r\n");
            printf("3.���DTU��STM32���ڵĽ����Ƿ���ȷ\r\n");
            printf("4.���DTU�����Ƿ�������DTU�Ƽ�ʹ��12V/1A��Դ���磬��Ҫʹ��USB��5V��ģ�鹩�磡��\r\n");
            printf("**************************************************************************\r\n\r\n");
            delay_ms(3000);
        }
    }
    printf("Cat1 DTU Init Success \r\n");

    dtu_rxlen = 0;
    RingBuffer_Reset(p_uart2_rxbuf);

    /*  
        DTU����͸��״̬�󣬾Ϳ��԰���������ͨ����ʹ�ã�����ȷ����1.Ӳ��������� 2.���ڲ�����DTU�ı���һ��
        ע�⣺DTUÿ���ϵ���Ҫһ����ʱ�䣬�ڵȴ����ӹ����У�MCU������DTU�������ݲ�������DTU�У��ȵ�������������ϣ�DTU���Զ������ݻ�������ȫ��ת�����������ϡ�
    */
//    while (1)
//    {
//        if (RingBuffer_Len(p_uart2_rxbuf) > 0)          /*���յ�DTU���͹����ķ���������*/
//        {
//            RingBuffer_Out(p_uart2_rxbuf, &buf, 1);

//            dtu_rxbuf[dtu_rxlen++] = buf;
//            dtu_get_urc_info(buf);                      /*����DTU�ϱ���URC��Ϣ*/

//            if (dtu_rxlen >= DTU_NETDATA_RX_BUF)        /*���ջ������*/
//            {
//                usart1_send_data(dtu_rxbuf, dtu_rxlen); /*���յ���DTU���������������ݣ�ת�������Դ���1���*/
//                dtu_rxlen = 0;
//            }
//        }
//        else
//        {
//            if (dtu_rxlen > 0)
//            {
//                usart1_send_data(dtu_rxbuf, dtu_rxlen); /*���յ���DTU���������������ݣ�ת�������Դ���1���*/
//                dtu_rxlen = 0;
//            }

//            LED0 = !LED0;
//            delay_ms(100);
//        }

//        key = KEY_Scan(0);
//        if(key == KEY0_PRES)
//        {
//            /*�����������ʽ��������Ҫ�����޸Ķ�Ӧ�����ݸ�ʽ������ֻ�������̲���ʹ��*/
//            send_data_to_dtu((uint8_t *)DTU_TEST_DATA, strlen(DTU_TEST_DATA));
//      
	
	delay_ms(5000);
	clrStruct();
////		TIM3_Int_Init(499,7199);					//50ms���ж�ʱ�䲻�ɹ������������ݣ�������ٶ�
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
		else if(mpu_dmp_get_data(&pitch,&roll,&yaw)==0)//dmp����õ����ݣ��Է���ֵ�����ж�
			{ 
//				temp=MPU_Get_Temperature();	                //�õ��¶�ֵ
//				MPU_Get_Accelerometer(&aacx,&aacy,&aacz);	//�õ����ٶȴ���������
//				MPU_Get_Gyroscope(&gyrox,&gyroy,&gyroz);	//�õ�����������
				
//				if(temp<0)								//�����������жϣ��ж�Ϊ��ʱ
//				{
//					temp=-temp;							//�Ը�����ȡ��
//				}
//				else                                    //�ж�Ϊ��ʱ
//				{
//				}
//				printf(" temp:%d.%d ",temp/100,temp%10); //ͨ������1����¶�
				
//				temp=pitch*10;							 //��tempΪpitch
				if(pitch<0)								//�����������жϣ��ж�Ϊ��ʱ
				{
					pitch=-pitch;						    //�Ը�����ȡ��		
				}
				else                                    //�ж�Ϊ��ʱ 
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
					 VIB2=1; //��ģ��
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
					 VIB1=1; //��ģ��
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
//		if(mpu_dmp_get_data(&pitch,&roll,&yaw)==0)//dmp����õ����ݣ��Է���ֵ�����ж�
//			{ 
////				temp=MPU_Get_Temperature();	                //�õ��¶�ֵ
////				MPU_Get_Accelerometer(&aacx,&aacy,&aacz);	//�õ����ٶȴ���������
////				MPU_Get_Gyroscope(&gyrox,&gyroy,&gyroz);	//�õ�����������
//				
////				if(temp<0)								//�����������жϣ��ж�Ϊ��ʱ
////				{
////					temp=-temp;							//�Ը�����ȡ��
////				}
////				else                                    //�ж�Ϊ��ʱ
////				{
////				}
////				printf(" temp:%d.%d ",temp/100,temp%10); //ͨ������1����¶�
//				
////				temp=pitch*10;							 //��tempΪpitch
//				if(pitch<0)								//�����������жϣ��ж�Ϊ��ʱ
//				{
//					pitch=-pitch;						    //�Ը�����ȡ��		
//				}
//				else                                    //�ж�Ϊ��ʱ 
//				{
//				}
////				double JIAO=0;
////				JIAO=temp/10;
////				temp=(temp%10)*0.1;
////				JIAO=JIAO+temp;
//			if (pitch>15)
//				{
////					 LED0 = 0; 
//					 VIB2=1; //��ģ��
//					}
//			else 
//			{
////				  LED0 = 1;
//				  VIB2=0;
//			}
////				printf(" pitch:%f ",pitch); //ͨ������1���pitch	
//				
////				temp=roll*10;                            //��tempΪroll
////				if(temp<0)								//�����������жϣ��ж�Ϊ��ʱ
////				{
////					temp=-temp;						    //�Ը�����ȡ��	
////				}
////				else                                    //�ж�Ϊ��ʱ
////				{
////				}
////				printf(" roll:%d.%d ",temp/10,temp%10);//ͨ������1���roll
////				
////				temp=yaw*10;                           //��tempΪyaw
////				if(temp<0)								//�����������жϣ��ж�Ϊ��ʱ
////				{
////					temp=-temp;						    //�Ը�����ȡ��
////				}
////				else                                    //�ж�Ϊ��ʱ
////				{
////				}
////				printf(" yaw:%d.%d \r\n",temp/10,temp%10);//ͨ������1���yaw	
//				
////				LED0=!LED0;								 			//LED��˸
//			}

//		    double distance = UltrasonicWave_Measure(); //????
//		    delay_ms(60);//??????? 60ms??, ????????????????
//			if (distance<20)
//				{
////					 LED0 = 0; 
//					 VIB1=1; //��ģ��
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
