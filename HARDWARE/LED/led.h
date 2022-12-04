#ifndef __LED_H
#define __LED_H	 
#include "sys.h"
//正点原子LED驱动
#define LED0 PCout(13)	// PC13
#define VIB1 PBout(8)	// PA7	振动模块
#define VIB2 PAout(8)	// PA7	振动模块

void LED_Init(void);//LED初始化

		 				    
#endif
