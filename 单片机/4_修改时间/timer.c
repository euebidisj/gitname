/***************�*************/
#include <stc15w.h>
#include <intrins.h>

/****************************** �û������ ***********************************/
#define		LED_TYPE	0x00	//����LED����, 0x00--����, 0xff--����	��

/************** Ԥ���� **************/
typedef 	unsigned char	u8;
typedef 	unsigned int	u16;
typedef 	unsigned long	u32; 

/*************	���س�������	**************/
u8 code t_display[]={						//��׼�ֿ�
//	 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
	0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71,
//black	 -     H    J	 K	  L	   N	o   P	 U     t    G    Q    r   M    y
	0x00,0x40,0x76,0x1E,0x70,0x38,0x37,0x5C,0x73,0x3E,0x78,0x3d,0x67,0x50,0x37,0x6e,
	0xBF,0x86,0xDB,0xCF,0xE6,0xED,0xFD,0x87,0xFF,0xEF,0x46,0x40};	//0. 1. 2. 3. 4. 5. 6. 7. 8. 9. -1

u8 code T_COM[]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};		//λ��


/*************	IO�ڶ���	**************/
sbit    SDA = P1^1;
sbit    SCL = P1^0;
sbit	LED7 = P1^7;
sbit	LED8 = P1^6;
sbit	LED9 = P4^7;
sbit	LED10 = P4^6;
sbit	P_HC595_SER   = P4^0;	//pin 14	SER		data input
sbit	P_HC595_RCLK  = P5^4;	//pin 12	RCLk	store (latch) clock
sbit	P_HC595_SRCLK = P4^3;	//pin 11	SRCLK	Shift data clock
sbit	save = P3^3;			//�������ݰ���

/*************	���ر�������	**************/
bit save_k;			//���水���ı�־λ
bit B_2ms;			//2ms��־λ
bit sec_bit;        //1s��־
u8 	LED_8[8];		//��ʾ����
u8	sun_8=0;		//��ʾλ����
u8 	stop_time = 1;//����ֹͣ��ʱ������ 
u8	time[3];		//��8563ͨѶʱ�õ�����
u8	key_z;			//��ֵ
u8	hour,minute,second;	//ʱ�����
u16 s_60=500;			//����led����

/********************** ��ʾʱ�Ӻ��� ************************/
void	DisplayRTC(void)
{
	if(hour >= 10)	LED_8[0] = hour / 10;
	else			LED_8[0] = 0x00;
	LED_8[1] = hour % 10;
	LED_8[2] = 0x11;
	LED_8[3] = minute / 10;
	LED_8[4] = minute % 10;
	LED_8[5] = 0X11;
	LED_8[6] = second / 10;
	LED_8[7] = second % 10;
}

/******************2������ʱ����********************/
void delay2us()		//@11.0592MHz
{
	unsigned char i;	  
	i = 3;
	while (--i);
}

/******************* 20ms��ʱ���� *******************/
void Delay20ms()		//@11.0592MHz
{
	unsigned char i, j, k;

	_nop_();
	_nop_();
	i = 1;
	j = 216;
	k = 35;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}

/*********************IIC�ӳ���***************************/
void IIC_start(void)		//��ʼ
{
//	delay2us();
	SCL = 1;
	SDA = 1;
	delay2us();
	SDA = 0;
	delay2us();
	SCL = 0;
	delay2us();
}

void IIC_stop(void)			//ֹͣ
{
	SDA = 0;
	delay2us();
	SCL = 1;
	delay2us();
	SDA = 1;
	delay2us();
	SCL = 0;
	delay2us();
	
}

void IIC_ACK(void)			//��Ӧ��
{
	SDA = 0;
	delay2us();
	SCL = 1;
	delay2us();
	SCL = 0;
	delay2us();
}

void IIC_no_ACK(void)		//������Ӧ��
{
	SDA = 1;
	delay2us();
	SCL = 1;
	delay2us();
	SCL = 0;
	delay2us();
}

void IIC_C_ACK(void)		//���Ӧ��
{
	SDA = 1;
	delay2us();
	SCL = 1;
	delay2us();
	F0 = SDA;
	SCL = 0;
	delay2us();
}

u8 IIC_R_byte(void)			//��һ���ֽ�
{
	u8 i = 8,dat = 0;
	SDA = 1;
	do 
	{
		SCL = 1;
		delay2us();
		dat <<= 1;
		if(SDA)	dat++;
		delay2us();
		SCL = 0;
		delay2us();
	}while(--i);
	return(dat);
}

void IIC_W_byte(u8 dat)		//дһ���ֽ�
{
	u8 i = 8;
	do
	{
		if(dat & 0x80)	SDA = 1;
		else 			SDA = 0;
		dat <<= 1;
		delay2us();
		SCL = 1;
		delay2us();
		SCL = 0;
		delay2us();
	}while(--i);	
}

void WriteNbyte(void)			//����дһ������
{
	u8 i=3;
	IIC_start();
	IIC_W_byte(0xA2);
	IIC_C_ACK();
	if(!F0)
	{
		IIC_W_byte(2);
		IIC_C_ACK();
		if(!F0)
		{	
			time[2] = ((second / 10) << 4) + (second % 10);		//time[2]--��
			time[1] = ((minute / 10) << 4) + (minute % 10);		//time[1]--��
			time[0] = ((hour / 10) << 4) + (hour % 10);			//time[0]--Сʱ
			do
			{
				IIC_W_byte(time[i - 1]);			//������time�������д��8563����time[2]��ʼ���Ͷ�ʱ�෴
				IIC_C_ACK();
				if(F0)	break;
			}while(--i);
		}
	}
	IIC_stop();
}

void read_time(void)  //������һ������
{
	IIC_start();
	IIC_W_byte(0xA2);
	IIC_C_ACK();
	if(!F0)
	{
		IIC_W_byte(2);
		IIC_C_ACK();
		if(!F0)
		{
			IIC_start();
			IIC_W_byte(0xA3);
			IIC_C_ACK();
			if(!F0)
			{		
				time[0] = IIC_R_byte();
				IIC_ACK();
				time[1] = IIC_R_byte();
				IIC_ACK();
				time[2] = IIC_R_byte();
				IIC_no_ACK();
			}
		}
	}
	IIC_stop();
	second = ((time[0] >> 4) & 0x07) * 10 + (time[0] & 0x0f);
	minute = ((time[1] >> 4) & 0x07) * 10 + (time[1] & 0x0f);
	hour   = ((time[2] >> 4) & 0x03) * 10 + (time[2] & 0x0f);
}

/*************** ��ʱ����ʼ�� *****************/
void Timer0Init(void)		//2����@11.0592MHz
{
	AUXR |= 0x80;		//��ʱ��ʱ��1Tģʽ
	TMOD &= 0xF0;		//���ö�ʱ��ģʽ
	TL0 = 0x9A;		//���ö�ʱ��ֵ
	TH0 = 0xA9;		//���ö�ʱ��ֵ
	TF0 = 0;		//���TF0��־
	TR0 = 1;		//��ʱ��0��ʼ��ʱ
	ET0=1;
}

/**************** ��HC595����һ���ֽں��� ******************/
void Send_595(u8 dat)
{		
	u8	i;
	for(i=0; i<8; i++)
	{
		dat <<= 1;
		P_HC595_SER   = CY;
		P_HC595_SRCLK = 1;
		P_HC595_SRCLK = 0;
	}
}

/********************** ��ʾɨ�躯�� ************************/
void DisplayScan(void)
{	
	Send_595(~LED_TYPE ^ T_COM[sun_8]);				//���λ��
	Send_595( LED_TYPE ^ t_display[LED_8[sun_8]]);	//�������

	P_HC595_RCLK = 1;
	P_HC595_RCLK = 0;								//�����������
	if(++sun_8 >= 8)	sun_8 = 0;					//8λ������0
}


/********************** Timer0 2ms�жϺ��� ************************/
void timer0 (void) interrupt 1
{
	DisplayScan();	//1msɨ����ʾ
	B_2ms = 1;		//2ms��־λ
	if(--s_60 == 0)
	{
		s_60 = 500;
		sec_bit = 1;
		LED10 = ~LED10;
	}
	if(s_60 == 250) LED10 = ~LED10;
}

/************************ ����ɨ�� ********************************/
void key_s()
{
	if(P0 != 0x0f)	Delay20ms();
	key_z = P0;
	if(save == 0)	Delay20ms();		//���水����ȥ��
	save_k = save;
}

/************************ ʱ���޸� ********************************/
void time_modify(void)
{
	if(!F0)						//�����жϰ����Ƿ��ɿ�
	{
		if(key_z != 0x0f)		//�ж��Ƿ��а�������
		{
			stop_time = 0;		//��ʱ��ֹͣ
			switch(key_z)		//����Ӽ�
			{
				case 0x0e: if(++hour >= 24)	hour = 0;  break;			//hour + 1
				case 0x0d: if(hour-- == 0)	hour = 23;  break;			//hour - 1
				case 0x0b: if(++minute >= 60)	minute = 0;  break;		//minute + 1
				case 0x07: if(minute-- == 0)	minute = 59;  break;	//minute - 1
			}
			F0 = 1;
			DisplayRTC();
		}
		if(!save_k)				//�ж��Ƿ񱣴�
		{
			second = 0;			//����㿪ʼ
			WriteNbyte();		//����д��8563
			stop_time = 1;		//��ʱ�����¿�ʼ
		}
	}
	else
	{
		if(key_z == 0x0f)	F0 = 0;		//�ж��ɿ������󣨰�F0��λ��Ϊ��һ�μӼ�ʱ����׼��
	}
}
/************************ ������ **********************************/
void main(void)
{
	u8 B_50ms;
	P0M0 = 0;		// IO�ڳ�ʼ��
	P0M1 = 0;
	P0 = 0x0f;		// ������̳�ʼ��
	
	Timer0Init();   // ��ʱ����ʼ��
	EA = 1;			//���жϿ���
	ET0 = 1;		//��ʱ��0�ж�����
	
//	time[0] = 0x12; //Сʱ 12 = 0001 0010
//	time[1] = 0;	//����
//	time[2] = 0;	//��
//	WriteNbyte();
	
	while(1)
	{
		if(B_2ms)
		{
			B_2ms = 0;
			if(sec_bit & stop_time)		//2ms��һ��ʱ��
			{
				sec_bit=0;
				read_time();
				DisplayRTC();
			}
			
			if(B_50ms++ >= 25)			//50msɨ��һ�ΰ���
			{
				B_50ms = 0;
				key_s();
			}
			time_modify();				//2msִ��һ��ʱ���޸ĳ���
		}
	}	
}




























