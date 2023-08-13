#include <reg52.h>
#include "stdio.h"
#include <string.h>
#include <stdlib.h>
#include <LCD1602.h>
#include "intrins.h"
//************************************
#define  uint  unsigned int
#define  uchar unsigned char
#define  Nack_counter  10
#define false 0
#define true 1
#define FOSC 11059200L		//系统时钟
#define BAUD 115200				//系统时钟
#define T0MS (65536-FOSC/12/500)		//500HZ in 12T MODE
#define ADC_POWER 0x80			//ADC POWER CONTROL BIT
#define ADC_FLAG 0x10			//ADC COMPLETE FLAG
#define ADC_START 0x08;			//ADC START CONTROL BIT
#define ADC_SPEEDLL 0x00		//540 CLOCKS
#define ADC_SPEEDL 0x20			//360 CLOCKS
#define ADC_SPEEDH 0x40			//180 CLOCKS
#define ADC_SPEEDHH 0x60		//90 CLOCKS
#define ADC_MASK 0x01
//GY906 端口定义
sbit  SCL=P1^3;// 时钟线
sbit  SDA=P1^4;// 数据线
sbit beep=P1^7; 			//蜂鸣器
sbit k1=P1^1; 			    //按键引脚     
sbit k2=P1^2; 			    
sbit k3=P1^5;
sbit k4=P1^6; 
//sbit k5=P1^7; 
//************ 数据定义****************
bdata uchar flag;//可位寻址数据
sbit bit_out=flag^7;
sbit bit_in=flag^0;
uchar DataH,DataL,Pecreg;
int hl=110,Mtemp = 38;
int wKey = 0;
unsigned char PulsePin = 0;       // Pulse Sensor purple wire connected to analog pin 0(P1.0????????????)
// these variables are volatile because they are used during the interrupt service routine!
volatile unsigned int BPM;                   // used to hold the pulse rate
volatile unsigned int Signal;                // holds the incoming raw data
volatile unsigned int IBI = 600;             // holds the time between beats, must be seeded! 
volatile bit Pulse = false;     // true when pulse wave is high, false when it's low
volatile bit QS = false;        // becomes true when Arduoino finds a beat.
volatile int rate[10];                    // array to hold last ten IBI values
volatile unsigned long sampleCounter = 0;          // used to determine pulse timing
volatile unsigned long lastBeatTime = 0;           // used to find IBI
volatile int Peak =512;                      // used to find peak in pulse wave, seeded
volatile int Trough = 512;                     // used to find trough in pulse wave, seeded
volatile int thresh = 512;                // used to find instant moment of heart beat, seeded
volatile int amp = 100;                   // used to hold amplitude of pulse waveform, seeded
int value  = 0;                 
volatile bit firstBeat = true;        // used to seed rate array so we startup with reasonable BPM
volatile bit secondBeat = false;      // used to seed rate array so we startup with reasonable BPM
unsigned char DisBuff[4]={0};

//************ 函数声明*****************************************
void   IIC_Start();                 //GY906 发起始位子程序
void   IIC_Stop();                 //GY906发结束位子程序
uchar  GY906_RX_Byte(void);              //GY906 接收字节子程序
void   GY906_tx_Bit(void);             //GY906发送位子程序
void   GY906_TX_Byte(uchar dat_byte);     //GY906 接收字节子程序
void   GY906_rx_Bit(void);           //GY906接收位子程序
int keyscan();                        //键盘函数
uint   memread(void);             // 读温度数据
void ADC_init(unsigned char channel);
void T0_init(void);
void convertToCelsius(uint Tem, uint* integerPart, uint* decimalPart);
void Delay_10us(unsigned int time)
{
	while(time--);
}

void sys_init()
{   
	ADC_init(PulsePin);
  T0_init();                 // sets up to read Pulse Sensor signal every 2mS  
	LCD_Init();
}
//*************主函数*******************************************
void main()
{
	
     uint Tem,tem1,tem2;
		uint i,addn,n = 1;
		uint Mflag = 0;
		sys_init();	
			beep = 0;
        //函数部分
        SCL=1;SDA=1;_nop_();
        _nop_();_nop_();_nop_();
        SCL=0;
        Delay(1000);
        SCL=1;			
        while(1)
        {		
					wKey = keyscan();
					if(wKey == 1)
					{
						if(Mflag % 2 == 0)
						{
							Mtemp++;
						}else{
							hl++;
						}
					}
					if(wKey == 2)
					{
						if(Mflag % 2 == 0)
						{
							Mtemp--;
						}else{
							hl--;
						}	
					}
					if(wKey == 3)
					{
						Mflag++;
					}
					if(wKey == 4)
					{
						n++;
						if(n%2 == 0)
						{
							addn = 0;
						}else
						{
							addn = 1;
						}
  					//LCD_ShowNum(1,8,4,1 );				
					}
					
//					if(wKey == 5)
//					{
//						Mflag = 122;	
//						//LCD_ShowNum(1,5,1,1 );	
//					}
					if(addn == 0)
					{
						LCD_ShowString(1,1,"                ");					
						LCD_ShowString(2,1,"                ");												
						LCD_ShowString(1,0," Welcome to use");
						Delay(138);
					}
					if(addn == 1)
					{
						if (QS == true){
						QS = false;	
						}
					Tem=memread();
					i = 0;
					value=0;
					while (DisBuff[i] == '\0') {
					i++;
					}
					for (; i < 3; i++) {
					if (DisBuff[i] >= '0' && DisBuff[i] <= '9') {
						value = value * 10 + (DisBuff[i] - '0');
					}
					}
					//value = (DisBuff[0] - '0') * 100 + (DisBuff[1] - '0') * 10 + (DisBuff[2] - '0');
//					for (i = 0; i < 3; i++) {
//					 if (DisBuff[i] >= '0' && DisBuff[i] <= '9') {
//						value = value * 10 + DisBuff[i];
//						}
//						else if (i == 0) {
//						value = value * 100;
//					}
					//}
					convertToCelsius(Tem,&tem1,&tem2);					
					Delay(138);						
						LCD_ShowString(1,1,"h:      t:      ");					//显示默认
					//LCD_ShowString(1,1,"h:             ");					//显示默认
					LCD_ShowNum(1,3,value,3);
					//LCD_ShowString(1,12,DisBuff);					//显示心率
						LCD_ShowNum(1,12,tem1,2);
					LCD_ShowString(1,14,".");					//显示心率
					LCD_ShowNum(1,15,tem2,2);
						LCD_ShowString(2,1,"Def H:     T:");					//显示阈值
						LCD_ShowNum(2,14,Mtemp,2 );
					  LCD_ShowNum(2,7,hl,3);
						if(tem1 > Mtemp||value>hl)
						{							
							beep = 1;
							Delay(1000);
							beep = 0;						
						}
					}
					
}			
}
void convertToCelsius(uint Tem,uint* integerPart, uint* decimalPart)
{ 
  uint T = Tem * 2;
    if (T >= 27315) 
    { 
        T = T - 27315; 
        *integerPart = T / 100; 
        *decimalPart = T % 100; 
    }
    else 
    {
        *integerPart = 0;
        *decimalPart = 0;
    }
}
//************************************
void IIC_Start(void)
{
   SDA=1;
   _nop_();_nop_();_nop_();_nop_();_nop_();
   SCL=1;
   _nop_();_nop_();_nop_();_nop_();_nop_();
   SDA=0;
   _nop_();_nop_();_nop_();_nop_();_nop_();
   SCL=0;
   _nop_();_nop_();_nop_();_nop_();_nop_();
}
//------------------------------
void IIC_Stop(void)
{

   SCL=0;
   _nop_();_nop_();_nop_();_nop_();_nop_();
   SDA=0;
   _nop_();_nop_();_nop_();_nop_();_nop_();
   SCL=1;
   _nop_();_nop_();_nop_();_nop_();_nop_();
   SDA=1;
}
//--------- 发送一个字节---------
void  GY906_TX_Byte(uchar dat_byte)
{
   char i,n,dat;
   n=Nack_counter;
TX_again:
   dat=dat_byte;
   for(i=0;i<8;i++)
   {
     if(dat&0x80)
      bit_out=1;
     else
      bit_out=0;
     GY906_tx_Bit();
     dat=dat<<1;
   }
   
      GY906_rx_Bit();
   if(bit_in==1)
   {
    IIC_Stop();
    if(n!=0)
    {n--;goto Repeat;}
    else
     goto exit;
   }
   else
    goto exit;
Repeat:
    IIC_Start();
    goto TX_again;
exit: ;
}
//-----------发送一个位---------
void  GY906_tx_Bit(void)
{
  if(bit_out==0) SDA=0;
  else SDA=1;
  _nop_();
  SCL=1;
  _nop_();_nop_();_nop_();_nop_();
  _nop_();_nop_();_nop_();_nop_();
  SCL=0;
  _nop_();_nop_();_nop_();_nop_();
  _nop_();_nop_();_nop_();_nop_();
}
//---------- 接收一个字节--------
uchar GY906_RX_Byte(void)
{
  uchar i,dat;
  dat=0;
  for(i=0;i<8;i++)
  {
    dat=dat<<1;
    GY906_rx_Bit();
    if(bit_in==1)
     dat=dat+1;
  }
  GY906_tx_Bit();
  return dat;
}
//---------- 接收一个位----------
void GY906_rx_Bit(void)
{
  SDA=1;bit_in=1;
  SCL=1;
  _nop_();_nop_();_nop_();_nop_();
  _nop_();_nop_();_nop_();_nop_();
  bit_in=SDA;
  _nop_();
  SCL=0;
  _nop_();_nop_();_nop_();_nop_();
  _nop_();_nop_();_nop_();_nop_();
}
//------------------------------
uint memread(void)
{
  IIC_Start();
  GY906_TX_Byte(0xB4);  //Send SlaveAddress ==============================
  //GY906_TX_Byte(0x00);
  GY906_TX_Byte(0x07);  //Send Command
  //------------
  IIC_Start();
  GY906_TX_Byte(0x01);
  bit_out=0;
  DataL=GY906_RX_Byte();
  bit_out=0;
  DataH=GY906_RX_Byte();
  bit_out=1;
  Pecreg=GY906_RX_Byte();
  IIC_Stop();
  return(DataH*256+DataL);
}
int keyscan()
{
	//----------------------按键检测---------------
	if(k1==0||k2==0||k3==0||k4==0)
	{
	if(k1==0)
	{
		Delay(20);
		if(k1==0)
		{
			return 1;
		}

	}
	if(k2==0)
	{
		Delay(20);
		if(k2==0)
		{
			return 2;
		}
	}
	if(k3==0)
	{
		Delay(20);
		if(k3==0)
		{
			return 3;
		}
	}
	if(k4==0)
	{
		Delay(20);
		if(k4==0)
		{
			return 4;
		}
	}
//	if(k5==0)
//	{
//		Delay(20);
//		if(k5==0)
//		{
//			return 5;
//		}
//	}
	}
	return 0;
}
void T0_init(void){     
  // Initializes Timer0 to throw an interrupt every 2mS.
	TMOD |= 0x01;	//16bit TIMER
	TL0=T0MS;
	TH0=T0MS>>8;
	TR0=1;		//start Timer 0
	ET0=1;		//enable Timer Interrupt
  EA=1;             // MAKE SURE GLOBAL INTERRUPTS ARE ENABLED      
} 

void ADC_init(unsigned char channel)
{
	P1ASF=ADC_MASK<<channel;	//enable PlusePin as ADC INPUT
	ADC_RES=0;	//clear former ADC result
	ADC_RESL=0;	//clear former ADC result
	AUXR1 |= 0x04;	//adjust the format of ADC result
	ADC_CONTR=channel|ADC_POWER|ADC_SPEEDLL|ADC_START;	//power on ADC and start conversion
}

unsigned int analogRead(unsigned char channel)
{
	unsigned int result;

	ADC_CONTR &=!ADC_FLAG;	//clear ADC FLAG
	result=ADC_RES;
	result=result<<8;
	result+=ADC_RESL;
	ADC_CONTR|=channel|ADC_POWER|ADC_SPEEDLL|ADC_START;
	return result;
}
// Timer 0?ж???????2MS?ж???Σ????AD????????????
void Timer0_rountine(void) interrupt 1
{                       
  int N;
	unsigned char i;
	// keep a running total of the last 10 IBI values
  unsigned int runningTotal = 0;                  // clear the runningTotal variable    

	EA=0;                                      // disable interrupts while we do this
	TL0=T0MS;
	TH0=T0MS>>8;				//reload 16 bit TIMER0
  Signal = analogRead(PulsePin);              // read the Pulse Sensor 
  sampleCounter += 2;                         // keep track of the time in mS with this variable
  N = sampleCounter - lastBeatTime;       // monitor the time since the last beat to avoid noise


    //  find the peak and trough of the pulse wave
  if(Signal < thresh && N > (IBI/5)*3){       // avoid dichrotic noise by waiting 3/5 of last IBI
    if (Signal < Trough){                        // T is the trough
      Trough = Signal;                         // keep track of lowest point in pulse wave 
    }
  }

  if(Signal > thresh && Signal > Peak){          // thresh condition helps avoid noise
    Peak = Signal;                             // P is the peak
  }                                        // keep track of highest point in pulse wave
  if (N > 250){                                   // avoid high frequency noise
    if ( (Signal > thresh) && (Pulse == false) && (N > (IBI/5)*3) ){        
      Pulse = true;                               // set the Pulse flag when we think there is a pulse
//      blinkPin=0;               // turn on pin 13 LED
      IBI = sampleCounter - lastBeatTime;         // measure time between beats in mS
      lastBeatTime = sampleCounter;               // keep track of time for next pulse

      if(secondBeat){                        // if this is the second beat, if secondBeat == TRUE
        secondBeat = false;                  // clear secondBeat flag
        for(i=0; i<=9; i++){             // seed the running total to get a realisitic BPM at startup
          rate[i] = IBI;                      
        }
      }
      if(firstBeat){                         // if it's the first time we found a beat, if firstBeat == TRUE
        firstBeat = false;                   // clear firstBeat flag
        secondBeat = true;                   // set the second beat flag
        EA=1;                               // enable interrupts again
        return;                              // IBI value is unreliable so discard it
      }   
      for(i=0; i<=8; i++){                // shift data in the rate array
        rate[i] = rate[i+1];                  // and drop the oldest IBI value 
        runningTotal += rate[i];              // add up the 9 oldest IBI values
      }

      rate[9] = IBI;                          // add the latest IBI to the rate array
      runningTotal += rate[9];                // add the latest IBI to runningTotal
      runningTotal /= 10;                     // average the last 10 IBI values 
      BPM = 60000/runningTotal;               // how many beats can fit into a minute? that's BPM!
			if(BPM>200)BPM=200;			//限制BPM最高显示值
			if(BPM<30)BPM=30;				///限制BPM最高显示值
			DisBuff[2]   = BPM%10+48;//???λ??
			DisBuff[1]   = BPM%100/10+48; //??λ??
			DisBuff[0]   = BPM/100+48;	   //??λ??
			if(DisBuff[0]==48)
				DisBuff[0]=32;
      QS = true;                              // set Quantified Self flag 
      // QS FLAG IS NOT CLEARED INSIDE THIS ISR
    }                       
  }
  if (Signal < thresh && Pulse == true){   // when the values are going down, the beat is over
//    blinkPin=1;            // turn off pin 13 LED
    Pulse = false;                         // reset the Pulse flag so we can do it again
    amp = Peak - Trough;                           // get amplitude of the pulse wave
    thresh = amp/2 + Trough;                    // set thresh at 50% of the amplitude
    Peak = thresh;                            // reset these for next time
    Trough = thresh;
  }
  if (N > 2500){                           // if 2.5 seconds go by without a beat
    thresh = 512;                          // set thresh default
    Peak = 512;                               // set P default
    Trough = 512;                               // set T default
    lastBeatTime = sampleCounter;          // bring the lastBeatTime up to date        
    firstBeat = true;                      // set these to avoid noise
    secondBeat = false;                    // when we get the heartbeat back
  }
  EA=1;                                   // enable interrupts when youre done!
}// end isr
