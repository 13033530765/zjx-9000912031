#include "timer.h"
#include "led.h"
#include "usart.h"
#include "ultra.h"
#include "delay.h"

//ͨ�ö�ʱ��3�жϳ�ʼ��
//����ʱ��ѡ��ΪAPB1��2������APB1Ϊ36M
//arr���Զ���װֵ��
//psc��ʱ��Ԥ��Ƶ��
void TIM3_Int_Init(u16 arr,u16 psc)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //ʱ��ʹ��
	
	//��ʱ��TIM3��ʼ��
	TIM_TimeBaseStructure.TIM_Period = arr; //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ	
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //����ʱ�ӷָ�:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM���ϼ���ģʽ
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //����ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
 
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE ); //ʹ��ָ����TIM3�ж�,��������ж�

	//�ж����ȼ�NVIC����
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  //TIM3�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;  //��ռ���ȼ�0��
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  //�����ȼ�3��
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQͨ����ʹ��
	NVIC_Init(&NVIC_InitStructure);  //��ʼ��NVIC�Ĵ���


	TIM_Cmd(TIM3, ENABLE);  //ʹ��TIMx
}
//��ʱ��3�жϷ������
void TIM3_IRQHandler(void)   //TIM3�ж�
{
	float distance;
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)  //���TIM3�����жϷ������
		{
			TIM_ClearITPendingBit(TIM3, TIM_IT_Update  );  //���TIMx�����жϱ�־ 
			if(mpu_dmp_get_data(&pitch,&roll,&yaw)==0)//dmp����õ����ݣ��Է���ֵ�����ж�
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
			if (pitch>10)
				{
//					 LED0 = 0; 
					 VIB2=1; //��ģ��
					}
			else 
			{
//				  LED0 = 1;
				  VIB2=0;
			}
//				printf(" pitch:%f ",pitch); //ͨ������1���pitch	
				
//				temp=roll*10;                            //��tempΪroll
//				if(temp<0)								//�����������жϣ��ж�Ϊ��ʱ
//				{
//					temp=-temp;						    //�Ը�����ȡ��	
//				}
//				else                                    //�ж�Ϊ��ʱ
//				{
//				}
//				printf(" roll:%d.%d ",temp/10,temp%10);//ͨ������1���roll
//				
//				temp=yaw*10;                           //��tempΪyaw
//				if(temp<0)								//�����������жϣ��ж�Ϊ��ʱ
//				{
//					temp=-temp;						    //�Ը�����ȡ��
//				}
//				else                                    //�ж�Ϊ��ʱ
//				{
//				}
//				printf(" yaw:%d.%d \r\n",temp/10,temp%10);//ͨ������1���yaw	
				
//				LED0=!LED0;								 			//LED��˸
			}

		    distance = UltrasonicWave_Measure(); //????
		    delay_ms(60);//??????? 60ms??, ????????????????
			if (distance<10)
				{
//					 LED0 = 0; 
					 VIB1=1; //��ģ��
					}
			else 
			{
//				  LED0 = 1;
				  VIB1=0;
			}
//		    printf("distance:%5.2f \r\n",distance);//?????
		
		}
		
		
}









