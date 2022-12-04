#ifndef __TIMER_H
#define __TIMER_H
#include "sys.h"
#include "mpu6050.h"//MPU6050驱动库
#include "inv_mpu.h"//陀螺仪驱动库
#include "inv_mpu_dmp_motion_driver.h" //DMP姿态解读库

extern float pitch,roll,yaw; 		//欧拉角:俯仰角，偏航角，滚转角
extern short aacx,aacy,aacz;		//加速度传感器原始数据  angular acceleration
extern short gyrox,gyroy,gyroz;	//陀螺仪原始数据  gyroscope
extern short temp;					//温度


void TIM3_Int_Init(u16 arr,u16 psc);	
 
#endif
