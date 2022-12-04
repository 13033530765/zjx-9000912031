#ifndef __TIMER_H
#define __TIMER_H
#include "sys.h"
#include "mpu6050.h"//MPU6050������
#include "inv_mpu.h"//������������
#include "inv_mpu_dmp_motion_driver.h" //DMP��̬�����

extern float pitch,roll,yaw; 		//ŷ����:�����ǣ�ƫ���ǣ���ת��
extern short aacx,aacy,aacz;		//���ٶȴ�����ԭʼ����  angular acceleration
extern short gyrox,gyroy,gyroz;	//������ԭʼ����  gyroscope
extern short temp;					//�¶�


void TIM3_Int_Init(u16 arr,u16 psc);	
 
#endif
