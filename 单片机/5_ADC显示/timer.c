/***************李海*************/
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
sbit	save = P3^3;			//保存数据按键

/*************	本地变量声明	**************/
bit B_2ms;			//2ms标志位
u8 	LED_8[8];		//显示缓冲
u8	sun_8=0;		//显示位索引
u8	key_z;			//键值
u16 adc;				

/********************** 显示时钟函数 ************************/
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

///******************2毫秒延时程序********************/
//void delay2us()		//@11.0592MHz
//{
//	unsigned char i;	  
//	i = 3;
//	while (--i);
//}


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
	P_HC595_RCLK = 0;								//锁存输出数据
	if(++sun_8 >= 8)	sun_8 = 0;					//8位结束回0
}

/*********************读adc**********************************/
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

/********************** Timer0 2ms中断函数 ************************/
void timer0 (void) interrupt 1
{
	DisplayScan();	//1ms扫描显示
	B_2ms = 1;		//2ms标志位
}

/************************ 主函数 **********************************/
void main(void)
{
	u8 B_10ms = 0;
	P0M0 = 0;		// IO口初始化
	P0M1 = 0;
	
	Timer0Init();   // 定时器初始化
	EA = 1;			//总中断开关
	ET0 = 1;		//定时器0中断允许
	
	P1ASF = 0x10;		//设置P1.4为模拟量输入功能
	ADC_CONTR = 0x81;	//打开A/D转换电源，设置输入通道
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
			if(++B_10ms >= 5)		//10ms扫描一次ADC键盘
			{
				B_10ms = 0;
				ADC_S();
			}
		}
	}	
}




























