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
bit B_2ms;			//2ms��־λ
u8 	LED_8[8];		//��ʾ����
u8	sun_8=0;		//��ʾλ����
u8	key_z;			//��ֵ
u16 adc;				

/********************** ��ʾʱ�Ӻ��� ************************/
void	DisplayRTC(void)
{
	LED_8[0] = adc / 1000;
	LED_8[1] = (adc / 100) % 10;
	LED_8[2] = (adc / 10) % 10;
	LED_8[3] = adc % 10;
	LED_8[4] = 0x10;
	LED_8[5] = 0X10;
	LED_8[6] = ((adc * 49) / 10000) + 32;
	LED_8[7] = ((adc * 49) / 1000) % 10;
}

///******************2������ʱ����********************/
//void delay2us()		//@11.0592MHz
//{
//	unsigned char i;	  
//	i = 3;
//	while (--i);
//}


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

/*********************��adc**********************************/
void ADC_S(void)
{
		ADC_CONTR = 0xEC;
		_nop_();
		_nop_();
		_nop_();
		_nop_();
		while(~ADC_CONTR & 0x10)
//		adc = (u16)(ADC_RES & 3);
//		adc = (adc << 8) | ADC_RESL;	
//		ADC_CONTR = 0x84;
		adc = ADC_RES;
		adc = (adc << 2) | (ADC_RESL & 3);
		DisplayRTC();
}

/********************** Timer0 2ms�жϺ��� ************************/
void timer0 (void) interrupt 1
{
	DisplayScan();	//1msɨ����ʾ
	B_2ms = 1;		//2ms��־λ
}

/************************ ������ **********************************/
void main(void)
{
	u8 B_10ms = 0;
	P0M0 = 0;		// IO�ڳ�ʼ��
	P0M1 = 0;
	
	Timer0Init();   // ��ʱ����ʼ��
	EA = 1;			//���жϿ���
	ET0 = 1;		//��ʱ��0�ж�����
	
	P1ASF = 0x10;		//����P1.4Ϊģ�������빦��
	ADC_CONTR = 0x81;	//��A/Dת����Դ����������ͨ��
//	AUXR1 ^= 0x04;
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	
	while(1)
	{
		if(B_2ms)
		{
			B_2ms = 0;
			if(++B_10ms >= 5)		//10msɨ��һ��ADC����
			{
				B_10ms = 0;
				ADC_S();
			}
		}
	}	
}




























