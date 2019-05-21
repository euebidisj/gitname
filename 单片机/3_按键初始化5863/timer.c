/***************完成时间_11:08*************/
#include <stc15w.h>
#include <intrins.h>

/****************************** 用户定义宏 ***********************************/
#define		LED_TYPE	0x00	//定义LED类型, 0x00--共阴, 0xff--共阳	秒

/************** 预定义 **************/
typedef 	unsigned char	u8;
typedef 	unsigned int	u16;
typedef 	unsigned long	u32; 

/*************	本地常量声明	**************/
u8 code t_display[]={						//标准字库
//	 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
	0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71,
//black	 -     H    J	 K	  L	   N	o   P	 U     t    G    Q    r   M    y
	0x00,0x40,0x76,0x1E,0x70,0x38,0x37,0x5C,0x73,0x3E,0x78,0x3d,0x67,0x50,0x37,0x6e,
	0xBF,0x86,0xDB,0xCF,0xE6,0xED,0xFD,0x87,0xFF,0xEF,0x46,0x40};	//0. 1. 2. 3. 4. 5. 6. 7. 8. 9. -1

u8 code T_COM[]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};		//位码


/*************	IO口定义	**************/
sbit    SDA = P1^1;
sbit    SCL = P1^0;
sbit	LED7 = P1^7;
sbit	LED8 = P1^6;
sbit	LED9 = P4^7;
sbit	LED10 = P4^6;
sbit	P_HC595_SER   = P4^0;	//pin 14	SER		data input
sbit	P_HC595_RCLK  = P5^4;	//pin 12	RCLk	store (latch) clock
sbit	P_HC595_SRCLK = P4^3;	//pin 11	SRCLK	Shift data clock

/*************	本地变量声明	**************/

u8 	LED_8[8];		//显示缓冲
u8	sun_8=0;		//显示位索引
bit sec_bit;        //1s标志 
u8 time[3];
u16 s_60=500;

/******************2毫秒延时程序********************/
void delay2us()		//@11.0592MHz
{
	unsigned char i;	  
	i = 3;
	while (--i);
}

/*********************IIC子程序***************************/
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

void WriteNbyte(void)			//写一串数据
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
				IIC_W_byte(time[i - 1]);			//将数组time里的初始值从time[2]开始
				IIC_C_ACK();
				if(F0)	break;
			}while(--i);
		}
	}
	IIC_stop();
}

void read_time(void)  //连续读一串数据
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

/*************** 定时器初始化 *****************/
void Timer0Init(void)		//2毫秒@11.0592MHz
{
	AUXR |= 0x80;		//定时器时钟1T模式
	TMOD &= 0xF0;		//设置定时器模式
	TL0 = 0x9A;		//设置定时初值
	TH0 = 0xA9;		//设置定时初值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
	ET0=1;
}

/**************** 向HC595发送一个字节函数 ******************/
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

/********************** 显示扫描函数 ************************/
void DisplayScan(void)
{	
	Send_595(~LED_TYPE ^ T_COM[sun_8]);				//输出位码
	Send_595( LED_TYPE ^ t_display[LED_8[sun_8]]);	//输出段码

	P_HC595_RCLK = 1;
	P_HC595_RCLK = 0;							//锁存输出数据
	if(++sun_8 >= 8)	sun_8 = 0;	//8位结束回0
}


/********************** Timer0 2ms中断函数 ************************/
void timer0 (void) interrupt 1
{
	DisplayScan();	//1ms扫描显示一位
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
	P0M0 = 0;		// IO口初始化
	P0M1 = 0;
	
	Timer0Init();   // 定时器初始化
	EA = 1;			//总中断开关
	ET0 = 1;		//定时器0中断允许
	
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
