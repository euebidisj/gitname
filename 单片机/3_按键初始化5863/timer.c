/***************�_���ʱ��_11:08*************/
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

/*************	���ر�������	**************/

u8 	LED_8[8];		//��ʾ����
u8	sun_8=0;		//��ʾλ����
bit sec_bit;        //1s��־ 
u8 time[3];
u16 s_60=500;

/******************2������ʱ����********************/
void delay2us()		//@11.0592MHz
{
	unsigned char i;	  
	i = 3;
	while (--i);
}

/*********************IIC�ӳ���***************************/
void IIC_start(void)
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

void IIC_stop(void)
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

void IIC_ACK(void)
{
	SDA = 0;
	delay2us();
	SCL = 1;
	delay2us();
	SCL = 0;
	delay2us();
}

void IIC_no_ACK(void)
{
	SDA = 1;
	delay2us();
	SCL = 1;
	delay2us();
	SCL = 0;
	delay2us();
}

void IIC_C_ACK(void)
{
	SDA = 1;
	delay2us();
	SCL = 1;
	delay2us();
	F0 = SDA;
	SCL = 0;
	delay2us();
}

u8 IIC_R_byte(void)
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

void IIC_W_byte(u8 dat)
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

void WriteNbyte(void)			//дһ������
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
		{	do
			{
				IIC_W_byte(time[i - 1]);			//������time��ĳ�ʼֵ��time[2]��ʼ
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

	LED_8[7] = time[0]&0x0f;
	LED_8[6] = (time[0]&0x7f)>>4;
	LED_8[5] = 43;
	LED_8[4] = time[1]&0x0f;
	LED_8[3] = (time[1]&0x7f)>>4;
	LED_8[2] = 43;
	LED_8[1] = time[2]&0x0f;
	LED_8[0] = (time[2]&0x3f)>>4; 
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
	P_HC595_RCLK = 0;							//�����������
	if(++sun_8 >= 8)	sun_8 = 0;	//8λ������0
}


/********************** Timer0 2ms�жϺ��� ************************/
void timer0 (void) interrupt 1
{
	DisplayScan();	//1msɨ����ʾһλ
	if(--s_60 == 0)
	{
		s_60 = 500;
		sec_bit = 1;
		LED10 = ~LED10;
	}
	if(s_60 == 250) LED10 = ~LED10;
}

void main(void)
{
	P0M0 = 0;		// IO�ڳ�ʼ��
	P0M1 = 0;
	
	Timer0Init();   // ��ʱ����ʼ��
	EA = 1;			//���жϿ���
	ET0 = 1;		//��ʱ��0�ж�����
	
	time[0] = 0x12;
	time[1] = 0;
	time[2] = 0;
	WriteNbyte();
	
	while(1)
	{
		if(sec_bit)
		{
			sec_bit=0;
			read_time();
		}		
	}	
}