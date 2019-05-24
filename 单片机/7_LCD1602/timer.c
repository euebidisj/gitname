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
u8 code t_display[]={						//标准字库
//	 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
	0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71,
//black	 -     H    J	 K	  L	   N	o   P	 U     t    G    Q    r   M    y
	0x00,0x40,0x76,0x1E,0x70,0x38,0x37,0x5C,0x73,0x3E,0x78,0x3d,0x67,0x50,0x37,0x6e,
	0xBF,0x86,0xDB,0xCF,0xE6,0xED,0xFD,0x87,0xFF,0xEF,0x46,0x40};	//0. 1. 2. 3. 4. 5. 6. 7. 8. 9. -1
u8 code asc[]={'0','1','2','3','4','5','6','7','8','9'};		//0-9的字符数组

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
sbit	Less = P3^3;			//减按键
sbit	Plus = P3^2;			//加按键
sbit 	EN = P2^7;				//已更正（因2.7口有问题，现改为）
sbit 	RW = P2^6;
sbit 	RS= P2^5;
//sbit	P2_3 = P2^4;

/*************	本地变量声明	**************/
bit B_2ms;			//2ms标志位
u8 	LED_8[8];		//显示缓冲
u8	sun_8=0;		//显示位索引
u8	key_z;			//键值
u8	Comparison;//比较值
u8	volt;		//键盘电压			
u16 adc;		//adc值
/************* 函数声明 ********************/
void  Q0();
//void  read_busy(void);

/***************** 读忙子程序 **************/
void read_busy(void)
{
	L1602_DATAPINS = 0xff;
	RS = 0;
	RW = 1;
	EN = 1;
	while (P0 & 0x80);					//P0和10000000相与，D7位若不为0，停在此处
	EN = 0;								 //若为0跳出进入下一步；这条语句的作用就是检测D7位
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

void LcdWriteCom(u16 com)	  //写入命令
{
	EN = 0;     //使能
	RS = 0;	   //选择发送命令
	
	L1602_DATAPINS = com;     //放入命令
	L1602_Delay1ms(1);		//等待数据稳定
	RW = 0;	   //选择写入

	EN = 1;	          //写入时序
	L1602_Delay1ms(5);	  //保持时间
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
	EN = 0;	//使能清零
	RS = 1;	//选择输入数据
	L1602_DATAPINS = dat; //写入数据
	L1602_Delay1ms(1);
	RW = 0;	//选择写入

	EN = 1;   //写入时序
	L1602_Delay1ms(5);   //保持时间
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


/*┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈
函数：擦除某一扇区（每个扇区512字节）
入口：addr = 某一扇区首地址                          
┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈*/
void cc(u16 addr)
{       
// 打开 IAP 功能(ISP_CONTR.7)=1:允许编程改变Flash, 设置Flash操作等待时间
// 0x83(晶振<5M)   0x82(晶振<10M)   0x81(晶振<20M)   0x80(晶振<40M)
    ISP_CONTR = 0x82;  
    ISP_CMD   = 0x03;                  // 用户可以对"Data Flash/EEPROM区"进行扇区擦除
    ISP_ADDRL = addr;         // ISP/IAP操作时的地址寄存器低八位，
    ISP_ADDRH = addr>>8;      // ISP/IAP操作时的地址寄存器高八位。
     EA =0;   
    ISP_TRIG = 0x5a;          // 在ISPEN(ISP_CONTR.7)=1时,对ISP_TRIG先写入46h，
    ISP_TRIG = 0xa5;          // 再写入B9h,ISP/IAP命令才会生效。
    _nop_();
    Q0();                                          // 关闭ISP/IAP
	EA =1;
}
/*┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈
函数：写一字节
入口：addr = 扇区单元地址 , dat = 待写入数据
┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈*/
void xcx(u16 addr,u8 dat)
{
    ISP_CONTR = 0x82;                  
    ISP_CMD   = 0x02;              // 用户可以对"Data Flash/EEPROM区"进行字节编程
    ISP_ADDRL = addr;        
    ISP_ADDRH = addr>>8;      
    ISP_DATA  = dat;          // 数据进ISP_DATA
 //   EA = 0;
    ISP_TRIG = 0x5a;         
    ISP_TRIG = 0xa5;         
    _nop_();
    Q0();                                          // 关闭ISP/IAP
//		EA =1;
}
/*┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈
函数：读一字节
入口：addr = 扇区单元地址
出口：dat  = 读出的数据
┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈*/
u8 dcx(u16 addr)
{   

    u8 dat;
       
    ISP_CONTR = 0x82;                  
    ISP_CMD   = 0x01;         // 用户可以对"Data Flash/EEPROM区"进行字节读
    ISP_ADDRL = addr;         
    ISP_ADDRH = addr>>8;      
//    EA = 0;
    ISP_TRIG = 0x5a;         
    ISP_TRIG = 0xa5;         
    _nop_();
    dat = ISP_DATA;                          // 取出数据
    Q0(); 													// 关闭ISP/IAP  
//		EA = 1;	
    return dat;
}
/*┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈
函数：关闭ISP/IAP操作
┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈*/
void Q0(void)
{
    ISP_CONTR = 0;            // 关闭IAP功能
    ISP_CMD   = 0;            // 待机模式，无ISP操作
    ISP_TRIG  = 0;            // 关闭IAP功能, 清与ISP有关的特殊功能寄存器
}

/********************** 显示函数 ************************/
void	DisplayRTC(void)
{
	LED_8[0] = adc / 1000;	LcdWriteCom(0x80);	LcdWriteData(asc[LED_8[0]]);
	LED_8[1] = (adc / 100) % 10;				LcdWriteData(asc[LED_8[1]]);
	LED_8[2] = (adc / 10) % 10;					LcdWriteData(asc[LED_8[2]]);
	LED_8[3] = adc % 10;						LcdWriteData(asc[LED_8[3]]);	LcdWriteCom(0x80+0x05);		
	LED_8[4] = (Comparison / 10) + 32;			LcdWriteData(asc[LED_8[4]-32]);	LcdWriteData('.');
	LED_8[5] = Comparison % 10;					LcdWriteData(asc[LED_8[5]]);	LcdWriteCom(0x80+0x09);
	LED_8[6] = ((adc * 49) / 10000) + 32;		LcdWriteData(asc[LED_8[6]-32]);	LcdWriteData('.');
	LED_8[7] = ((adc * 49) / 1000) % 10;		LcdWriteData(asc[LED_8[7]]);
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

/*****************200ms延时程序 *******************/
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
		while(~ADC_CONTR & 0x10)					//等待adc键盘读完
		adc = ADC_RES;								//先取出高八位放到adc
		adc = (adc << 2) | (ADC_RESL & 3);			//后取出低两位放到adc
		DisplayRTC();								//显示数据
		volt = (adc * 49) / 1000;					//将ADC值转成X10的电压值
}

/********************* 读加减按键 ******************************/
void lp_s(void)
{
	if(!Less)										//判断减按键是否按下
	{
//		F0 = 0;
		Delay20ms();								//消抖延时
		if(++Comparison >= 50)	Comparison = 0;		//比较值加一
		Delay200ms();								//长按连加延时
//	while(!Less);
		F0 = 1;										//存储数据
	}
	
	if(!Plus)										//判断加按键是否按下
	{
//		F0 = 0;
		Delay20ms();								//消抖延时
		if(Comparison-- == 0)	Comparison = 50;	//比较值减一
		Delay200ms();								//长按连减延时
//	while(!Plus);
		F0 = 1;										//存储数据
//		p = 0;
	}   
}				

/********************** Timer0 2ms中断函数 ************************/
void timer0 (void) interrupt 1
{
	DisplayScan();	//2ms扫描显示
	B_2ms = 1;		//2ms标志位
}

/********************** test *************************************/
void LcdDisplay()
{
//	Set_Disp_Pos(0x40);
//	Write_LCD_Data('0');
//	Display_LCD_String(0x40,"feughf");
	LcdWriteCom(0x80+0x40);
	LcdWriteData('9');				//时
	LcdWriteData('A');				 
	LcdWriteData('-');
	LcdWriteData('0');				//分
	LcdWriteData('0');	
	LcdWriteData('-');
	LcdWriteData('0');				//秒
	LcdWriteData('0');

	LcdWriteCom(0x80);
	LcdWriteData('2');
	LcdWriteData('0');
	LcdWriteData('0');			//年
	LcdWriteData('0');
	LcdWriteData('-');
	LcdWriteData('0');			//月
	LcdWriteData('0');
	LcdWriteData('-');
	LcdWriteData('0');			//日
	LcdWriteData('0');
	LcdWriteCom(0x8D);
	LcdWriteData('0');	//星期			 
}
/************************ 主函数 **********************************/
void main(void)
{
	u8 B_10ms = 0;
	u8 i;
	u16 B_3s = 0;
	P0M0 = 0;		// IO口初始化
	P0M1 = 0;
	P2M0 = 0;
	P2M1 = 0;		//设置成双向口
	
	Timer0Init();   // 定时器初始化
	EA = 1;			//总中断开关
	ET0 = 1;		//定时器0中断允许
	Comparison = dcx(0xe600);		//读取上次设置的比较值
	
	P1ASF = 0x10;		//设置P1.4为模拟量输入功能
	ADC_CONTR = 0x81;	//打开A/D转换电源，设置输入通道
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	F0 = 0;

	LcdInit();			//查看资料说明，不应开机立即执行lcd初始化。
	
	while(1)
	{
		if(B_2ms)
		{
			B_2ms = 0;
			if(++B_10ms >= 5)		//10ms扫描一次键盘
			{
				B_10ms = 0;
				ADC_S();			//读ADC
				lp_s();				//读加减按键
			}
			
			if(volt > Comparison)	LED10 = 0;			//比较电压和比较值并显示led10
			else	LED10 = 1;
			
			if(F0)										//判断是否有按键按下
			{
				if(++B_3s >= 1500)
				{
					B_3s = 0;
					cc(0xe600);							//擦除e600h扇区
					xcx(0xe600,Comparison);				//数据写入e600h扇区
					for(i=0;i<=4;i++)
					{
						LED9 = !LED9;					//切换LED9状态
						Delay200ms();					//每隔200ms改变一次状态
					}	
					LED9 = 1;							//恢复LED9状态，灭
					F0 = 0;								//F0复位为下次闪烁做准备
				}
			}
			else	B_3s = 0;							//保证存储的是最后一次数据
		}
	}	
}