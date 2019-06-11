/*************** 2019_05_19 *************/
#include <stc15w.h>
#include <intrins.h>
//#include <LCD1602.h>

/****************************** 用户定义宏 ***********************************/
#define		LED_TYPE	0x00					//定义LED类型, 0x00--共阴, 0xff--共阳	秒
#define 	L1602_DATAPINS  P0		//数据端口

/****************特殊功能寄存器声明****************/
sfr ISP_DATA = 0xc2;   
sfr ISP_ADDRH = 0xc3;     
sfr ISP_ADDRL = 0xc4;   
sfr ISP_CMD = 0xc5;   
sfr ISP_TRIG = 0xc6;      
sfr ISP_CONTR = 0xc7;

/************** 预定义 **************/
typedef 	unsigned char	u8;
typedef 	unsigned int	u16;
typedef 	unsigned long	u32; 

/*************	本地常量声明	**************/
u8 code asc[]={'0','1','2','3','4','5','6','7','8','9'};		//0-9的字符数组
u8 code asc_1[]={"Line  detectorpower by 2"};

/*************	IO口定义	**************/
sbit    SDA = P1^1;
sbit    SCL = P1^0;
sbit	P_HC595_SER   = P4^0;	//pin 14	SER		data input
sbit	P_HC595_RCLK  = P5^4;	//pin 12	RCLk	store (latch) clock
sbit	P_HC595_SRCLK = P4^3;	//pin 11	SRCLK	Shift data clock
sbit	FY = P3^2;				//左移动/开始
sbit	FY1 = P3^3;				//右移动
sbit 	EN = P2^7;				//已更正（因2.7口有问题，现改为）
sbit 	RW = P2^6;
sbit 	RS= P2^5;
sbit    G_C = P1^2;             //灯检测的端口
sbit    F_M = P1^1;             //蜂鸣器控制端口
sbit	H_W_F = P1^3;			//红外发射控制端口

/*************	本地变量声明	**************/
bit B_2ms;			//2ms标志位
u8 	xdata R_R[48];			//存储线状态数组
u8 	xdata R_P_R[17];
u8 	xdata R_P_G[17];			
u8	w_z;			//方格位置变量	
u8	th1;
u8	g,h;			//有线的格的数量
u8	f;
u16 adc;			//adc值
/************* 函数声明 ********************/
void  Q0();

/***************** 读忙子程序 **************/
void read_busy(void)
{
	L1602_DATAPINS = 0xff;
	RS = 0;
	RW = 1;
	EN = 1;
	while (P0 & 0x80);						//P0和10000000相与，D7位若不为0，停在此处
	EN = 0;									//若为0跳出进入下一步；这条语句的作用就是检测D7位
}

/*******************************************************************************
* 函 数 名         : L1602_Delay1ms
* 函数功能		   : 延时函数，延时1ms
* 输    入         : c
* 输    出         : 无
* 说    名         : 该函数是在12MHZ晶振下，12分频单片机的延时。
*******************************************************************************/

void L1602_Delay1ms(u16 c)   //误差 0us
{
    u8 a,b;
	for (; c>0; c--)
	{
		for (b=199;b>0;b--)
		{
			for(a=1;a>0;a--);
		}      
	} 	
}

/*******************************************************************************
* 函 数 名         : LcdWriteCom
* 函数功能		   : 向LCD写入一个字节的命令
* 输    入         : com
* 输    出         : 无
*******************************************************************************/

void LcdWriteCom(u16 com)	 	//写入命令
{
	EN = 0;     				//使能
	RS = 0;	   					//选择发送命令
	
	L1602_DATAPINS = com;     	//放入命令
	L1602_Delay1ms(1);			//等待数据稳定
	RW = 0;	   					//选择写入

	EN = 1;	         	 		//写入时序
	L1602_Delay1ms(5);	  		//保持时间
	EN = 0;
}

/*******************************************************************************
* 函 数 名         : LcdWriteData
* 函数功能		   : 向LCD写入一个字节的数据
* 输    入         : dat
* 输    出         : 无
*******************************************************************************/		    
void LcdWriteData(u16 dat)			//写入数据
{
	EN = 0;							//使能清零
	RS = 1;							//选择输入数据
	L1602_DATAPINS = dat; 			//写入数据
	L1602_Delay1ms(1);
	RW = 0;							//选择写入

	EN = 1;   						//写入时序
	L1602_Delay1ms(5);   			//保持时间
	EN = 0;
}

/*******************************************************************************
* 函 数 名       : LcdInit()
* 函数功能		 : 初始化LCD屏
* 输    入       : 无
* 输    出       : 无
*******************************************************************************/		   
void LcdInit()						  //LCD初始化子程序
{
	L1602_Delay1ms(15);
 	LcdWriteCom(0x38);  //开显示
	L1602_Delay1ms(5);
	LcdWriteCom(0x38);  //开显示 
	L1602_Delay1ms(5);
	LcdWriteCom(0x38);  //开显示 
	L1602_Delay1ms(5);
	LcdWriteCom(0x0c);  //开显示不显示光标
	read_busy();
	LcdWriteCom(0x04);  //写一个指针加1
	read_busy();
	LcdWriteCom(0x01);  //清屏
	read_busy();
	LcdWriteCom(0x80);  //设置数据指针起点
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
		adc = ADC_RES;
		adc = (adc << 2) | (ADC_RESL & 3);
		LcdWriteCom(0x80+0x40);	
		LcdWriteData(asc[adc/1000]);
		LcdWriteData(asc[adc/100%10]);
		LcdWriteData(asc[adc/10%10]);
		LcdWriteData(asc[adc%10]);
}

/*****************延时程序 *******************/
void Delay200ms()		//@11.0592MHz
{
	unsigned char i, j, k;

	_nop_();
	_nop_();
	i = 9;
	j = 104;
	k = 139;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}
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


/*********************** 显示左/右移动 *****************************/
void Display1(void)
{
	u8 i;
	if (F0)
	{
		F0 = 0;
		LcdWriteCom(0x01);					//清屏
		LcdWriteCom(0x80);
		LcdWriteData('R');
		LcdWriteData(':');
		LcdWriteCom(0x80+0x40);
		LcdWriteData('J');
		LcdWriteData(':');
	}
	if (f + 5 < g)							//日光灯的左移
	{
		LcdWriteCom(0x80+0x02);				
		for ( i = f; i < f + 5; i++)		//执行5次
		{
			LcdWriteData(asc[R_P_R[i]/10]);
			LcdWriteData(asc[R_P_R[i]%10]);
			LcdWriteData('/');
		}		
	}	
	if (f + 5 < h)							//节能灯的左移
	{
		LcdWriteCom(0x80+0x42);
		for ( i = f; i < f + 5; i++)		//执行5次
		{
			LcdWriteData(asc[R_P_G[i]/10]);
			LcdWriteData(asc[R_P_G[i]%10]);
			LcdWriteData('/');
		}		
	}
	if(g > h)
	{
		if(f + 5 >= g)	f = g - 5;
	}
	else
	{
		if(f + 5 >= h)	f = h - 5;
	}	
}

void Display2(void)
{
	u8 i;
	if (f >= 0)							//日光灯的右移
	{
		LcdWriteCom(0x80+0x02);				
		for ( i = f; i < f + 5; i++)		//执行5次
		{
			LcdWriteData(asc[R_P_R[i]/10]);
			LcdWriteData(asc[R_P_R[i]%10]);
			LcdWriteData('/');
		}		
	}	
	if (f >= 0)							//节能灯的右移
	{
		LcdWriteCom(0x80+0x42);
		for ( i = f; i < f + 5; i++)		//执行5次
		{
			LcdWriteData(asc[R_P_G[i]/10]);
			LcdWriteData(asc[R_P_G[i]%10]);
			LcdWriteData('/');
		}		
	}
//	if(f == 0)	f = 1;
}

/*********************** 显示线格 *********************************/
void Display(void)
{
	u8 L = 0,i;
	LcdWriteCom(0x01);					//清屏
	LcdWriteCom(0x80);
	LcdWriteData('R');
	LcdWriteData(':');
	for ( i = 0; i < 48; i++)
	{
		if (R_R[i] == 1)				
		{
			if(g < 5)
			{
				LcdWriteData(asc[i/10]);
				LcdWriteData(asc[i%10]);
				LcdWriteData('/');
			}
			R_P_R[L++] = i;
			g++;
		}	
	}
	LcdWriteCom(0x80+0x40);
	LcdWriteData('J');
	LcdWriteData(':');
	L = 0;
	for ( i = 0; i < 48; i++)
	{
		if (R_R[i] == 2)
		{
			if(h < 5)
			{
				LcdWriteData(asc[i/10]);
				LcdWriteData(asc[i%10]);
				LcdWriteData('/');
			}
			R_P_G[L++] = i;
			h++;
		}
	}
}

/********************* 位置检测程序 *******************************/
u8 w_detection(void)
{
	bit a;
	if (adc < 2)	
	{
		w_z++;		//方格数+1
		a = 1;
	}
	else	a = 0;
	LcdWriteCom(0x80+0x00);
	LcdWriteData('G');
	LcdWriteData(asc[w_z/10]);
	LcdWriteData(asc[w_z%10]);
	LcdWriteCom(0x80+0x04);
	LcdWriteData('R');
	LcdWriteCom(0x80+0x08);
	LcdWriteData('J');
	return a;
}

/********************** 线状态检测程序 ****************************/
void d_detection(void)                
{
	bit z = 1,x =1;
	u8 i,j;
	th1 = 0;
	TH1=0;   					//定时器高位，初值设为0
	TL1=0;   					//定时器低位，初值设为0
	TF1=0;  					//定时器溢出次数，初值设为0
	while(G_C);  				//pulse为脉冲的输入引脚
	if (G_C)
	{
tt:		TR1=1;       				//打开定时器
		while(G_C);   				//等待下降沿来临
		while(!G_C);    			//等待上升沿来临
		TR1=0;						//关闭定时器
		th1=500000/(TH1*256+TL1);	//记录值
	}
	else
	{
		i = 54;
		j = 199;
		do
		{
			while (--j);
			if(G_C)	goto tt;
		} while (--i);
	}
	LcdWriteCom(0x80+0x45);
	LcdWriteData(asc[th1/100%10]);
	LcdWriteData(asc[th1/10%10]);
	LcdWriteData(asc[th1%10]);
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
void Timer1Init(void)		//500微秒@11.0592MHz
{
	TMOD &= 0x09;		//设置定时器模式
	TL1 = 0;		//设置定时初值
	TH1 = 0;		//设置定时初值
	TF1 = 0;		//清除TF1标志

}

/********************** Timer0 2ms中断函数 ************************/
void timer0 (void) interrupt 1
{
	B_2ms = 1;		//2ms标志位
}

/************************ 主函数 **********************************/
void main(void)
{
	bit one;
	bit on = 1;
	bit TWO = 1;
	u8 i = 0,c = 0,v = 0,b = 0,n,m;
	H_W_F = 0;
	B_2ms = 0;
    F_M = 1;
    F0 = 1;
	G_C = 1;
	P0M0 = 0;						// IO口初始化
	P0M1 = 0;
	P2M0 = 0;
	P2M1 = 0;						//设置成双向口

	LcdInit();						//查看资料说明，不应开机立即执行lcd初始化。
	LcdWriteCom(0x80+0x01);
	for (i = 0; i < 24; i++)        //开机显示内容
	{
		if (i == 14)		LcdWriteCom(0x80+0x45);
		LcdWriteData(asc_1[i]);     
	}
		
	Timer0Init();   				//定时器0初始化
	Timer1Init();					//定时器1初始化
	
	P1ASF = 0x10;					//设置P1.4为模拟量输入功能
	ADC_CONTR = 0x81;				//打开A/D转换电源，设置输入通道

	i = 0;
	one = 1;

	while(1)
	{
		if (!FY & one)					//开始程序
		{
			LcdWriteCom(0x01);
			EA = 1;						//总中断开关
			ET0 = 1;					//定时器0中断允许
    		ET1 = 1;        			//定时器1中断允许
			one = 0;					//开机后执行一次后不在执行
		}

		if(B_2ms)
		{
			B_2ms = 0;
			if(w_z < 49)
			{
				if (++m > 250)						//500ms检测一次位置
				{
					m = 0;
					if(w_detection())					//位置检测程序
					{
						Delay200ms();
					}
				}				
				if (++i >= 25)						//50ms执行一次
				{
					i = 0;
					H_W_F = 1;						//红外发射
					ADC_S();						//检测红外接收电压
					H_W_F = 0;						//红外关闭
					d_detection();
				}
				if (++c >= 25)						//50ms执行一次,线位置存储
				{
					if(w_z > n)	on = 1;
					if (th1 > 15 & on)
					{
						n = w_z;
						on = 0;
						R_R[w_z-1] = 1;
						LcdWriteCom(0x80+0x05);
						++v;
						LcdWriteData(asc[v/10]);
						LcdWriteData(asc[v%10]);
						if (th1 >35)
						{
							R_R[w_z-1] = 2;
							LcdWriteCom(0x80+0x09);
							++b;
							LcdWriteData(asc[b/10]);
							LcdWriteData(asc[b%10]);
							LcdWriteCom(0x80+0x05);
							--v;
							LcdWriteData(asc[v/10]);
							LcdWriteData(asc[v%10]);
						}					
					}
					if(th1 >15)	F_M = 0;
					else		F_M = 1;
				}
			}
			else
			{
				if (TWO)
				{
					TWO = 0;
					Display();			//显示线格
				}
				if(!FY)					//左移
				{
					f++;				//显示线格的数组位置加一
					Delay20ms();
					Display1();
					Delay200ms();
				}
				if (!FY1)				//右移
				{
					if(f != 0) f--;				//显示线格的数组位置减一
					Delay20ms();
					Display2();
					Delay200ms();
				}				
			}			
		}
	}	
}