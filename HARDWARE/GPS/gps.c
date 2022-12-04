#include "delay.h"
#include "usart.h"
#include "led.h"
#include "string.h"
#include <stdio.h>  
#include "atk_m750.h"
#include "gps.h"

char cell_phone_number[11]={"13586239740"};
extern _SaveData Save_Data;
void errorLog(int num)
{
	
	while (1)
	{
	  	printf("ERROR%d\r\n",num);
	}
}

void parseGpsBuffer()
{
	char *subString;
	char *subStringNext;
	char i = 0;
	if (Save_Data.isGetData)
	{
		Save_Data.isGetData = false;
		printf("**************\r\n");
		printf(Save_Data.GPS_Buffer);

		
		for (i = 0 ; i <= 6 ; i++)
		{
			if (i == 0)
			{
				if ((subString = strstr(Save_Data.GPS_Buffer, ",")) == NULL)
					errorLog(1);	//��������
			}
			else
			{
				subString++;
				if ((subStringNext = strstr(subString, ",")) != NULL)
				{
					char usefullBuffer[2]; 
					switch(i)
					{
						case 1:memcpy(Save_Data.UTCTime, subString, subStringNext - subString);break;	//��ȡUTCʱ��
						case 2:memcpy(usefullBuffer, subString, subStringNext - subString);break;	//��ȡUTCʱ��
						case 3:memcpy(Save_Data.latitude, subString, subStringNext - subString);break;	//��ȡγ����Ϣ
						case 4:memcpy(Save_Data.N_S, subString, subStringNext - subString);break;	//��ȡN/S
						case 5:memcpy(Save_Data.longitude, subString, subStringNext - subString);break;	//��ȡ������Ϣ
						case 6:memcpy(Save_Data.E_W, subString, subStringNext - subString);break;	//��ȡE/W

						default:break;
					}

					subString = subStringNext;
					Save_Data.isParseData = true;
					if(usefullBuffer[0] == 'A')
						Save_Data.isUsefull = true;
					else if(usefullBuffer[0] == 'V')
						Save_Data.isUsefull = false;

				}
				else
				{
					errorLog(2);	//��������
				}
			}


		}
	}
}
//m^n����
//����ֵ:m^n�η�.
u32 NMEA_Pow(u8 m,u8 n)
{
	u32 result=1;	 
	while(n--)result*=m;    
	return result;
}
//strת��Ϊ����,��','����'*'����
//buf:���ִ洢��
//dx:С����λ��,���ظ����ú���
//����ֵ:ת�������ֵ
int NMEA_Str2num(u8 *buf,u8*dx)
{
	u8 *p=buf;
	u32 ires=0,fres=0;
	u8 ilen=0,flen=0,i;
	u8 mask=0;
	int res;
	while(1) //�õ�������С���ĳ���
	{
		if(*p=='-'){mask|=0X02;p++;}//�Ǹ���
		if(*p=='\0')break;//����������
		if(*p=='.'){mask|=0X01;p++;}//����С������
		else if(*p>'9'||(*p<'0'))	//�зǷ��ַ�
		{	
			ilen=0;
			flen=0;
			break;
		}	
		if(mask&0X01)flen++;
		else ilen++;
		p++;
	}
	if(mask&0X02)buf++;	//ȥ������
	for(i=0;i<ilen;i++)	//�õ�������������
	{  
		ires+=NMEA_Pow(10,ilen-1-i)*(buf[i]-'0');
	}
	if(flen>5)flen=5;	//���ȡ5λС��
	*dx=flen;	 		//С����λ��
	for(i=0;i<flen;i++)	//�õ�С����������
	{  
		fres+=NMEA_Pow(10,flen-1-i)*(buf[ilen+1+i]-'0');
	} 
	res=ires*NMEA_Pow(10,flen)+fres;
	if(mask&0X02)res=-res;		   
	return res;
}




void sendGpsBuffer()
{
//		printf("in");
//		Save_Data.isParseData = true;
	if (Save_Data.isParseData)
	{
//				printf("in1");
		Save_Data.isParseData = false;
				printf(Save_Data.GPS_Buffer);

//		printf("Save_Data.UTCTime = ");
//		printf(Save_Data.UTCTime);
//		printf("\r\n");
		u8 dx;
		if(Save_Data.isUsefull)
		{ 
			double temp=0,t2=0,wd=0,jd=0;
			int t1=0;
			Save_Data.isUsefull = false;
//			printf("WD: ");
			temp=NMEA_Str2num(Save_Data.latitude,&dx);
			temp/=100000;
			t1=(int)(temp/100);
			t2=(temp-t1*100)/60;
			wd=(double)(t1+t2);
			char dtu_smg_buf[1024];
//			printf("%f",wd);
////			printf("_");
//			
//			printf(Save_Data.N_S);
//			printf("\r\n");

//			printf("JD= ");
		  temp=NMEA_Str2num(Save_Data.longitude,&dx);
			temp/=100000;
			t1=(int)(temp/100);
			t2=(temp-t1*100)/60;
			jd=t1+t2;
//			printf("%f",temp);
////			printf("_");
//			printf(Save_Data.E_W);
//			printf("\r\n");
			
			snprintf(dtu_smg_buf, DTU_SMS_SEND_BUF_MAX, "%s%s%s%f%s%s%s%s%f%s%s","GPS_Location" ,"                 ","WD:", wd,"_",Save_Data.N_S,"                  " ,"JD:", jd,"_",Save_Data.E_W);
			printf(dtu_smg_buf);
			dtu_send_sms(cell_phone_number, dtu_smg_buf);
//			printf("ok");
		}
		else
		{
			printf("GPS DATA is not usefull!\r\n");
			
		}
	}
	
	} 
void printGpsBuffer()
{
	if (Save_Data.isParseData)
	{
		Save_Data.isParseData = false;
		
//		printf("Save_Data.UTCTime = ");
//		printf(Save_Data.UTCTime);
//		printf("\r\n");
		u8 dx;
		if(Save_Data.isUsefull)
		{ 
			double temp=0,t2=0,wd=0,jd=0;
			int t1=0;
			Save_Data.isUsefull = false;
			printf("WD: ");
			temp=NMEA_Str2num(Save_Data.latitude,&dx);
			temp/=100000;
			t1=(int)(temp/100);
			t2=(temp-t1*100)/60;
			wd=(double)(t1+t2);
			static char dtu_smg_buf[1024];
			printf("%f",wd);
			printf("_");
			
			printf(Save_Data.N_S);
			printf("\r\n");

			printf("JD= ");
		  temp=NMEA_Str2num(Save_Data.longitude,&dx);
			temp/=100000;
			t1=(int)(temp/100);
			t2=(temp-t1*100)/60;
			temp=t1+t2;
			printf("%f",temp);
			printf("_");
			printf(Save_Data.E_W);
			printf("\r\n");
			
		}
		else
		{
			printf("GPS DATA is not usefull!\r\n");
		}
		
	}
}
