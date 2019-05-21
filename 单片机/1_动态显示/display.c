//*****代码练习

#include	<stc15w.h>		//单片机头文件

/**************定义宏***************/
#define		RCT_ENABLE	1	

#define		LED8_TYPE	0x00		//共阴数码管

#define		Timer0_Reload	(65536UL -(MAIN_Fosc / 1000))



/**************本地常量声明***************/
u8 code t_display[]={			//标准字库
	0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71,
	0x00,0x40,0x76,0x1E,0x70,0x38,0x37,0x5C,0x73,0x3E,0x78,0x3d,0x67,0x50,0x37,0x6e,
	0xBF,0x86,0xDB,0xCF,0xE6,0xED,0xFD,0x87,0xFF,0xEF,0x46
};


u8 code T_COM[]={
	0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80
};								//位码


/**************IO口定义***********/
sbit	P_HC595_SER		= P4^0;
sbit	P_HC595_RCLK	= P5^4;
sbit	P_HC595_SRCLK	= P4^3;

/*************	本地变量声明	**************/
u8 	LED8[8];		//显示缓冲
u8	shu;			
u8	shu_8;		//显示位索引
bit	B_1ms;			//1ms标志

u16	msecond;	//用于记录定时器溢出次数，控制时间

/************* 本地变量声明  *********/

/*************  加法  ****************/
void RTC(void)
{
	if(shu++ >= 15)	shu = 0;	//0~9循环，和归为
}
/*************  显示  ****************/
void	DisplayRTC(void)
{
	u8 i = 0;
	for(i = 0 ; i < 8 ; i++)	LED8[i] = shu;	//把shu的值放到LED[]
}

/*********  主函数  **********/
int main (void)
{
	u8 i;
	shu = 0;
	shu_8 = 0;
	
	Timer0_1T();
	Timer0_AsTimer();
	Timer0_16bitAutoReload();
	Timer0_Load(Timer0_Reload);
	Timer0_InterruptEnable();
	Timer0_Run();
	EA = 1;
	
	for(i=0; i<8; i++)	LED8[i] = i;	//显示01234567
	
	while(1)
	{
		if(B_1ms)
		{
			B_1ms =0;
			if(++msecond >= 500)
			{
				msecond = 0;
				DisplayRTC();
				RTC();
			}
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
	Send_595( LED8_TYPE ^ t_display[LED8[shu_8]]);	//输出段码

	P_HC595_RCLK = 1;
	P_HC595_RCLK = 0;							//锁存输出数据
	if(++shu_8 >= 8)	shu_8 = 0;	//8位结束回0
}

/********************** Timer0 1ms中断函数 ************************/
void timer0 (void) interrupt TIMER0_VECTOR
{
	DisplayScan();	//1ms扫描显示一位
	B_1ms = 1;		//1ms标志
	
}
