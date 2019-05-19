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
bit save_k;			//保存按键的标志位
bit B_2ms;			//2ms标志位
bit sec_bit;        //1s标志
u8 	LED_8[8];		//显示缓冲
u8	sun_8=0;		//显示位索引
u8 	stop_time = 1;//用于停止读时钟数据 
u8	time[3];		//和8563通讯时用的数组
u8	key_z;			//键值
u8	hour,minute,second;	//时间变量
u16 s_60=500;			//控制led秒闪

/********************** 显示时钟函数 ************************/
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

/******************2毫秒延时程序********************/
void delay2us()		//@11.0592MHz
{
	unsigned char i;	  
	i = 3;
	while (--i);
}

/******************* 20ms延时程序 *******************/
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

/*********************IIC子程序***************************/
void IIC_start(void)		//开始
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

void IIC_stop(void)			//停止
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

void IIC_ACK(void)			//发应答
{
	SDA = 0;
	delay2us();
	SCL = 1;
	delay2us();
	SCL = 0;
	delay2us();
}

void IIC_no_ACK(void)		//不发送应答
{
	SDA = 1;
	delay2us();
	SCL = 1;
	delay2us();
	SCL = 0;
	delay2us();
}

void IIC_C_ACK(void)		//检查应答
{
	SDA = 1;
	delay2us();
	SCL = 1;
	delay2us();
	F0 = SDA;
	SCL = 0;
	delay2us();
}

u8 IIC_R_byte(void)			//读一个字节
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

void IIC_W_byte(u8 dat)		//写一个字节
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

void WriteNbyte(void)			//连续写一串数据
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
			time[2] = ((second / 10) << 4) + (second % 10);		//time[2]--秒
			time[1] = ((minute / 10) << 4) + (minute % 10);		//time[1]--分
			time[0] = ((hour / 10) << 4) + (hour % 10);			//time[0]--小时
			do
			{
				IIC_W_byte(time[i - 1]);			//将数组time里的数据写到8563，从time[2]开始，和读时相反
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
	second = ((time[0] >> 4) & 0x07) * 10 + (time[0] & 0x0f);
	minute = ((time[1] >> 4) & 0x07) * 10 + (time[1] & 0x0f);
	hour   = ((time[2] >> 4) & 0x03) * 10 + (time[2] & 0x0f);
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
	P_HC595_RCLK = 0;								//锁存输出数据
	if(++sun_8 >= 8)	sun_8 = 0;					//8位结束回0
}


/********************** Timer0 2ms中断函数 ************************/
void timer0 (void) interrupt 1
{
	DisplayScan();	//1ms扫描显示
	B_2ms = 1;		//2ms标志位
	if(--s_60 == 0)
	{
		s_60 = 500;
		sec_bit = 1;
		LED10 = ~LED10;
	}
	if(s_60 == 250) LED10 = ~LED10;
}

/************************ 键盘扫描 ********************************/
void key_s()
{
	if(P0 != 0x0f)	Delay20ms();
	key_z = P0;
	if(save == 0)	Delay20ms();		//保存按键的去抖
	save_k = save;
}

/************************ 时间修改 ********************************/
void time_modify(void)
{
	if(!F0)						//用于判断按键是否松开
	{
		if(key_z != 0x0f)		//判断是否有按键按下
		{
			stop_time = 0;		//读时间停止
			switch(key_z)		//处理加减
			{
				case 0x0e: if(++hour >= 24)	hour = 0;  break;			//hour + 1
				case 0x0d: if(hour-- == 0)	hour = 23;  break;			//hour - 1
				case 0x0b: if(++minute >= 60)	minute = 0;  break;		//minute + 1
				case 0x07: if(minute-- == 0)	minute = 59;  break;	//minute - 1
			}
			F0 = 1;
			DisplayRTC();
		}
		if(!save_k)				//判断是否保存
		{
			second = 0;			//秒从零开始
			WriteNbyte();		//数据写回8563
			stop_time = 1;		//读时间重新开始
		}
	}
	else
	{
		if(key_z == 0x0f)	F0 = 0;		//判断松开按键后（把F0复位）为下一次加减时间做准备
	}
}
/************************ 主函数 **********************************/
void main(void)
{
	u8 B_50ms;
	P0M0 = 0;		// IO口初始化
	P0M1 = 0;
	P0 = 0x0f;		// 矩阵键盘初始化
	
	Timer0Init();   // 定时器初始化
	EA = 1;			//总中断开关
	ET0 = 1;		//定时器0中断允许
	
//	time[0] = 0x12; //小时 12 = 0001 0010
//	time[1] = 0;	//分钟
//	time[2] = 0;	//秒
//	WriteNbyte();
	
	while(1)
	{
		if(B_2ms)
		{
			B_2ms = 0;
			if(sec_bit & stop_time)		//2ms读一次时间
			{
				sec_bit=0;
				read_time();
				DisplayRTC();
			}
			
			if(B_50ms++ >= 25)			//50ms扫描一次按键
			{
				B_50ms = 0;
				key_s();
			}
			time_modify();				//2ms执行一次时间修改程序
		}
	}	
}




























