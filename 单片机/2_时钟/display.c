//*********时钟
#include <stc15w.h>
#include <intrins.h>


/**************定义宏***************/
//#define		RCT_ENABLE	1	

#define		LED8_TYPE	0x00		//共阴数码管
#define 	DIS_			0x11
#define 	DIS_BLACK	0x10
#define 	SLAW			0xA2
#define 	SLAR			0xA3
#define MAIN_Fosc		22118400L	//定义主时钟


/************** 预定义 **************/
typedef 	unsigned char	u8;
typedef 	unsigned int	u16;
typedef 	unsigned long	u32;


/************ I0口定义 **************/
sbit LED7 = P1^7;
sbit LED8 = P1^6;
sbit LED9 = P4^7;
sbit LED10 = P4^6;
//sbit dit = P0^4;
sbit k0 = P0^0;

sbit	SDA	= P1^1;	//定义SDA  PIN5
sbit	SCL	= P1^0;	//定义SCL  PIN6

sbit	P_HC595_SER   = P4^0;	//pin 14	SER		data input
sbit	P_HC595_RCLK  = P5^4;	//pin 12	RCLk	store (latch) clock
sbit	P_HC595_SRCLK = P4^3;	//pin 11	SRCLK	Shift data clock
//sbit sLED= P2^7;


/**************本地常量声明***************/
u8 code t_display[]={			//标准字库
	0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71,
	0x00,0x40,0x76,0x1E,0x70,0x38,0x37,0x5C,0x73,0x3E,0x78,0x3d,0x67,0x50,0x37,0x6e,
	0xBF,0x86,0xDB,0xCF,0xE6,0xED,0xFD,0x87,0xFF,0xEF,0x46
};


u8 code T_COM[]={
	0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80			//位码
};

/*************	本地变量声明	**************/
u8 		LED_8[8];							//显示缓冲
u8		shu_8;								//显示/显示索引
u8		hour,minute,second;  	//时间变量
bit		B_1ms;								//1ms标志
//bit	B_1s;									//1s标志

u16	msecond_500ms;	
u16	msecond_1s;							//用于记录定时器溢出次数，控制时间

/************* 延时20ms ********************/
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


/*************** 定时器初始化 *****************/
void Timer0Init(void)				//1毫秒@11.0592MHz
{
	AUXR &= 0x7F;						//定时器时钟12T模式
	TMOD &= 0xF0;						//设置定时器模式
	TL0 = 0x66;							//设置定时初值
	TH0 = 0xFC;							//设置定时初值
	TF0 = 0;								//清除TF0标志
	TR0 = 1;								//定时器0开始计时
}


/********************** 显示时钟函数 ************************/
void	DisplayRTC(void)
{
	if(hour >= 10)	LED_8[0] = hour / 10;
	else			LED_8[0] = DIS_BLACK;
	LED_8[1] = hour % 10;
	LED_8[2] = DIS_;
	LED_8[3] = minute / 10;
	LED_8[4] = minute % 10;
	LED_8[5] = DIS_;
	LED_8[6] = second / 10;
	LED_8[7] = second % 10;
}


/************** RF8563通讯程序 **************************/
void delay2us(void)		//@11.0592MHz
{
	u8	dly;
	dly = MAIN_Fosc / 2000000UL;		//按2us计算
	while(--dly)	;
}
//{
//	u8 i = 3;
//	while (--i);
//}

void IIC_start(void)		//开始
{					
	SDA = 1;
	delay2us();
	SCL = 1;
	delay2us();
	SDA = 0;
	delay2us();
	SCL = 0;
	delay2us();
}
void IIC_stop(void)			//停止
{
	SDA = 0;
	delay2us();
	SCL = 1;
	delay2us();
	SDA = 1;
	delay2us();
}
void sent_ack()				//有应答
{
	SDA = 0;
	delay2us();
	SCL = 1;
	delay2us();
	SCL = 0;
	delay2us();
}
void sent_no_ack()			//无应答
{ 
	SDA = 1;
	delay2us();
	SCL = 1;
	delay2us();
	SCL = 0;
	delay2us();
}
void check_ack()			//发应答
{
	SDA = 1;
	delay2us();
	SCL = 1;
	delay2us();
	F0  = SDA;
	SCL = 0;
	delay2us();
}

void writebyte(u8 dat)		//写数据
{
	u8 i;
	i = 8;
	do
	{
		if(dat & 0x80)	SDA = 1;
		else			SDA	= 0;
		dat <<= 1;
		delay2us();
		SCL = 1;
		delay2us();
		SCL = 0;
		delay2us();
	}
	while(--i);
}

u8 readbyte(void)			//读数据
{
	u8 i,dat;
	i = 8;
	SDA = 1;
	do
	{
		SCL = 1;
		delay2us();
		dat <<= 1;
		if(SDA)		dat++;
		SCL  = 0;
		delay2us();
	}
	while(--i);
	return(dat);
}

void WriteNbyte(u8 addr, u8 *p, u8 number)			//写一串数据
{
	IIC_start();
	writebyte(SLAW);
	check_ack();
	if(!F0)
	{
		writebyte(addr);
		check_ack();
		if(!F0)
		{
			do
			{
				writebyte(*p);		p++;
				check_ack();
				if(F0)	break;
			}
			while(--number);
		}
	}
	IIC_stop();
}

void ReadNbyte(u8 addr, u8 *p, u8 number)		//读一串数据
{
	IIC_start();
	writebyte(SLAW);
	check_ack();
	if(!F0)
	{
		writebyte(addr);
		check_ack();
		if(!F0)
		{
			IIC_start();
			writebyte(SLAR);
			check_ack();
			if(!F0)
			{
				do
				{
					*p = readbyte();	p++;
					if(number != 1)		sent_ack();	//有应答
				}
				while(--number);
				sent_no_ack();						//无应答
			}
		}
	}
	IIC_stop();
}

/********************** 读RTC8563函数 ************************/
void	ReadRTC(void)
{
	u8	tmp[3];

	ReadNbyte(2, tmp, 3);
	second = ((tmp[0] >> 4) & 0x07) * 10 + (tmp[0] & 0x0f);
	minute = ((tmp[1] >> 4) & 0x07) * 10 + (tmp[1] & 0x0f);
	hour   = ((tmp[2] >> 4) & 0x03) * 10 + (tmp[2] & 0x0f);
}

/********************** 写RTC8563函数 ************************/
void	WriteRTC(void)
{
	u8	tmp[3];

	tmp[0] = ((second / 10) << 4) + (second % 10);
	tmp[1] = ((minute / 10) << 4) + (minute % 10);
	tmp[2] = ((hour / 10) << 4) + (hour % 10);
	WriteNbyte(2, tmp, 3);
}

///********************** 按键扫描 *****************************/
//u8 key_s()
//{
//	if(P0 != 0x0f) Delay20ms();
//	return(P0);
//}

///********************* 按键处理 ******************************/
//void LED_8_S(u8 dat)
//{
//	switch(dat)
//	{
//		case 0x0e: LED_8[7] = 1; break;
//		case 0x0d: LED_8[7] = 2; break;
//		case 0x0b: LED_8[7] = 3; break;
//		case 0x07: LED_8[7] = 4; break;
//		case 0x0f: LED_8[7] = 0; break;
//	}		
//}
/********************  主程序 ********************/
void main(void)
{
	u8 z;
	u16  msecond_500ms;
//	u16  msecond_1s;
	B_1ms = 0;
	P0 = 0x0f; 

	shu_8 = 0;
	LED10 = 1;
	
	P0M0 = 0;		// IO口初始化
	P0M1 = 0;
	
	Timer0Init();   // 定时器初始化
	EA = 1;			//总中断开关
	ET0 = 1;		//定时器0中断允许
	
//	for(p=0;p<8;p++)	LED_8[p] = p;	//显示01234567
		
	ReadRTC();
//	F0 = 0;
//	if(second >= 60)	F0 = 1;	//错误
//	if(minute >= 60)	F0 = 1;	//错误
//	if(hour   >= 60)	F0 = 1;	//错误
//	if(F0)						//有错误, 默认12:00:00
//	{
		second = 0;
		minute = 0;
		hour  = 12;
		WriteRTC();
//	}

	DisplayRTC();
	for(z=0;z<8;z++)	LED_8[z] = z;	//显示01234567
//	for(z=0;z<8;z++)	LED_8[z] = 0;
	
	while(1)
	{
		if(B_1ms)						//一秒扫描一次
		{
			B_1ms = 0;			//1ms标志复位
			if(msecond_500ms++ > 500)
			{
				msecond_500ms = 0;
				LED10 = ~LED10;
			}
			if(msecond_1s++ > 1000)
			{
				msecond_1s = 0;			//1s标志复位
				ReadRTC();				//从8563读数据更新时间数据到时间变量
				DisplayRTC();			//更新数据到显示缓冲数组
			}
		}
		if(!k0)	
		{
			Delay20ms();
			if(!k0)	LED9 = !LED9;
			while(!k0);	
			Delay20ms();
		}
	}
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
	Send_595(~LED8_TYPE ^ T_COM[shu_8]);				//输出位码
	Send_595( LED8_TYPE ^ t_display[LED_8[shu_8]]);	//输出段码

	P_HC595_RCLK = 1;
	P_HC595_RCLK = 0;							//锁存输出数据
	if(++shu_8 >= 8)	shu_8 = 0;	//8位结束回0
}

/********************** Timer0 1ms中断函数 ************************/
void Timer0() interrupt 1
{
	DisplayScan();	//1ms扫描显示一位
	B_1ms = 1;		//1ms标志
}								
