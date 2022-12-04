#ifndef __USART_H
#define __USART_H
#include "stdio.h"    
#include "sys.h" 
#include "string.h"
//////////////////////////////////////////////////////////////////////////////////     
//������ֻ��ѧϰʹ�ã�δ���������ɣ��������������κ���;
//ALIENTEK STM32������
//����1��ʼ��           
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2012/8/18
//�汾��V1.5
//��Ȩ���У�����ؾ���
//Copyright(C) �������������ӿƼ����޹�˾ 2009-2019
//All rights reserved
//********************************************************************************
//V1.3�޸�˵�� 
//֧����Ӧ��ͬƵ���µĴ��ڲ���������.
//�����˶�printf��֧��
//�����˴��ڽ��������.
//������printf��һ���ַ���ʧ��bug
//V1.4�޸�˵��
//1,�޸Ĵ��ڳ�ʼ��IO��bug
//2,�޸���USART_RX_STA,ʹ�ô����������ֽ���Ϊ2��14�η�
//3,������USART_REC_LEN,���ڶ��崮������������յ��ֽ���(������2��14�η�)
//4,�޸���EN_USART1_RX��ʹ�ܷ�ʽ
//V1.5�޸�˵��
//1,�����˶�UCOSII��֧��





/***************************************************************************************************************/
/*                                            ATK-GPRS-M26���̵���                                               */
/***************************************************************************************************************/
#define ATK_DTU_Printf(fmt,...)    \
    do{\
        printf("[Slave] : ");\
        printf(fmt,##__VA_ARGS__);\
        printf("\r\n");\
    }while(0)





#define USART_REC_LEN              200      //�����������ֽ��� 200
#define EN_USART1_RX             1        //ʹ�ܣ�1��/��ֹ��0������1����
          
extern u8  USART_RX_BUF[USART_REC_LEN]; //���ջ���,���USART_REC_LEN���ֽ�.ĩ�ֽ�Ϊ���з� 
extern u16 USART_RX_STA;                 //����״̬���    
//����봮���жϽ��գ��벻Ҫע�����º궨��
void uart_init(u32 bound);

void usart1_send_data(u8 *data, u32 size);
		
#define false 0
#define true 1		
//�������鳤��
#define GPS_Buffer_Length 80
#define UTCTime_Length 11
#define latitude_Length 11
#define N_S_Length 2
#define longitude_Length 12
#define E_W_Length 2 		

typedef struct SaveData 
{
	char GPS_Buffer[GPS_Buffer_Length];
	char isGetData;		//�Ƿ��ȡ��GPS����
	char isParseData;	//�Ƿ�������
	char UTCTime[UTCTime_Length];		//UTCʱ��
	char latitude[latitude_Length];		//γ��
	char N_S[N_S_Length];		//N/S
	char longitude[longitude_Length];		//����
	char E_W[E_W_Length];		//E/W
	char isUsefull;		//��λ��Ϣ�Ƿ���Ч
} _SaveData;

extern char rxdatabufer;
extern u16 point1;
void CLR_Buf(void);
u8 Hand(char *a);
void clrStruct(void);
#endif

