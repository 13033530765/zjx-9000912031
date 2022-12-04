#include "key.h"
#include "delay.h"
//////////////////////////////////////////////////////////////////////////////////	 
//±¾³ÌĞòÖ»¹©Ñ§Ï°Ê¹ÓÃ£¬Î´¾­×÷ÕßĞí¿É£¬²»µÃÓÃÓÚÆäËüÈÎºÎÓÃÍ¾
//ALIENTEK Mini STM32¿ª·¢°å
//°´¼üÊäÈë Çı¶¯´úÂë		   
//ÕıµãÔ­×Ó@ALIENTEK
//¼¼ÊõÂÛÌ³:www.openedv.com
//ĞŞ¸ÄÈÕÆÚ:2014/3/06
//°æ±¾£ºV1.0
//°æÈ¨ËùÓĞ£¬µÁ°æ±Ø¾¿¡£
//Copyright(C) ¹ãÖİÊĞĞÇÒíµç×Ó¿Æ¼¼ÓĞÏŞ¹«Ë¾ 2009-2019
//All rights reserved									   
//////////////////////////////////////////////////////////////////////////////////	 
 	    
//°´¼ü³õÊ¼»¯º¯Êı 
//PA15º ÉèÖÃ³ÉÊäÈë
void KEY_Init(void)
{
	
	GPIO_InitTypeDef GPIO_InitStructure;

 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);//Ê¹ÄÜPORTA,PORTCÊ±ÖÓ

	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);//¹Ø±Õjtag£¬Ê¹ÄÜSWD£¬¿ÉÒÔÓÃSWDÄ£Ê½µ÷ÊÔ
	
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_15;//PA15
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //ÉèÖÃ³ÉÉÏÀ­ÊäÈë
 	GPIO_Init(GPIOA, &GPIO_InitStructure);//³õÊ¼»¯GPIOA15
	

	
} 
//°´¼ü´¦Àíº¯Êı
//·µ»Ø°´¼üÖµ
//mode:0,²»Ö§³ÖÁ¬Ğø°´;1,Ö§³ÖÁ¬Ğø°´;
//·µ»ØÖµ£º
//0£¬Ã»ÓĞÈÎºÎ°´¼ü°´ÏÂ
//KEY0_PRES£¬KEY0°´ÏÂ
//KEY1_PRES£¬KEY1°´ÏÂ
//WKUP_PRES£¬WK_UP°´ÏÂ 
//×¢Òâ´Ëº¯ÊıÓĞÏìÓ¦ÓÅÏÈ¼¶,KEY0>KEY1>WK_UP!!
u8 KEY_Scan(u8 mode)
{	 
	static u8 key_up=1;//°´¼ü°´ËÉ¿ª±êÖ¾
	if(mode)key_up=1;  //Ö§³ÖÁ¬°´		  
	if(key_up&&KEY==0)
	{
		delay_ms(10);//È¥¶¶¶¯ 
		key_up=0;
		if(KEY==0)return 1;
		
	}else if(KEY==1)key_up=1; 	     
	return 0;// ÎŞ°´¼ü°´ÏÂ
}
