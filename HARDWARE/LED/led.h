#ifndef __LED_H
#define __LED_H	 
#include "sys.h"
//����ԭ��LED����
#define LED0 PCout(13)	// PC13
#define VIB1 PBout(8)	// PA7	��ģ��
#define VIB2 PAout(8)	// PA7	��ģ��

void LED_Init(void);//LED��ʼ��

		 				    
#endif
