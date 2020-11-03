/**
  Generated Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This is the main file generated using PIC10 / PIC12 / PIC16 / PIC18 MCUs

  Description:
    This header file provides implementations for driver APIs for all modules selected in the GUI.
    Generation Information :
        Product Revision  :  PIC10 / PIC12 / PIC16 / PIC18 MCUs - 1.65.2
        Device            :  PIC16F18875
        Driver Version    :  2.00
*/

/*
    (c) 2018 Microchip Technology Inc. and its subsidiaries. 
    
    Subject to your compliance with these terms, you may use Microchip software and any 
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party 
    license terms applicable to your use of third party software (including open source software) that 
    may accompany Microchip software.
    
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER 
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY 
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS 
    FOR A PARTICULAR PURPOSE.
    
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP 
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO 
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL 
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT 
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS 
    SOFTWARE.
*/

#include "mcc_generated_files/mcc.h"
#include "I2C/i2c.h"
//#include "stdio.h"
#include "stdlib.h"
#include <string.h>



#define S2DELAY 100 //Use to define time to wait for debounce

#define EEAddr_INIT    0x7000        // EEPROM starting address (Used for storing init data)
#define EEAddr_RING    0x7014        // Ring Buffer starting address

/*
                         Main application
 */

uint8_t NREG = 0;   //Number of registers
uint8_t PMON = 0;   //Monitoring period //////////////// MISSING
uint8_t TALA = 0;   //Duration of alarm signal (PWM)
/*uint8_t ALAH = 0;   //Hours of alarm clock
uint8_t ALAM = 0;   //Minutes of alarm clock
uint8_t ALAS = 0;   //Seconds of alarm clock
uint8_t ALAT = 0;   //Alarm threshold for Temperature
uint8_t ALAL = 0;*/   //Alarm threshold for Luminosity
uint8_t ALAF = 0;   //Alarm Flag (Initially disabled)
/*uint8_t CLKH = 0;   //Initial value for clock hours
uint8_t CLKM = 0;*/   //Initial value for clock minutes
uint8_t idx_RingBuffer = 0; //Index of Ring Buffer to EEPROM

bool S1_Value = false;

unsigned char tsttc (void)
{
	unsigned char value;
do{
	IdleI2C();
	StartI2C(); IdleI2C();
    
	WriteI2C(0x9a | 0x00); IdleI2C();
	WriteI2C(0x01); IdleI2C();
	RestartI2C(); IdleI2C();
	WriteI2C(0x9a | 0x01); IdleI2C();
	value = ReadI2C(); IdleI2C();
	NotAckI2C(); IdleI2C();
	StopI2C();
} while (!(value & 0x40));

	IdleI2C();
	StartI2C(); IdleI2C();
	WriteI2C(0x9a | 0x00); IdleI2C();
	WriteI2C(0x00); IdleI2C();
	RestartI2C(); IdleI2C();
	WriteI2C(0x9a | 0x01); IdleI2C();
	value = ReadI2C(); IdleI2C();
	NotAckI2C(); IdleI2C();
	StopI2C();

	return value;
}

#define LCD_ADDR 0x4e   // 0x27 << 1
#define LCD_BL 0x08
#define LCD_EN 0x04
#define LCD_RW 0x02
#define LCD_RS 0x01

void LCDsend(unsigned char c)
{
  IdleI2C();
  StartI2C(); IdleI2C();
  WriteI2C(LCD_ADDR | 0x00); IdleI2C();
  WriteI2C(c); IdleI2C();
  StopI2C();    
}
unsigned char LCDrecv(unsigned char mode)
{
  unsigned char hc;
  unsigned char lc;
  
  IdleI2C();
  StartI2C(); IdleI2C();
  WriteI2C(LCD_ADDR | 0x00); IdleI2C();
  WriteI2C(0xf0 | LCD_BL | LCD_RW | mode); IdleI2C();
  WriteI2C(0xf0 | LCD_BL | LCD_EN | LCD_RW | mode); IdleI2C();
  __delay_us(1);
  RestartI2C(); IdleI2C();
  WriteI2C(LCD_ADDR | 0x01); IdleI2C();
  hc = ReadI2C(); IdleI2C();
  NotAckI2C();
  RestartI2C(); IdleI2C();
  WriteI2C(LCD_ADDR | 0x00); IdleI2C();
  WriteI2C(0xf0 | LCD_BL | LCD_RW | mode); IdleI2C();
  WriteI2C(0xf0 | LCD_BL | LCD_EN | LCD_RW | mode); IdleI2C();
  __delay_us(1);
  RestartI2C(); IdleI2C();
  WriteI2C(LCD_ADDR | 0x01); IdleI2C();
  lc = ReadI2C(); IdleI2C();
  NotAckI2C();
  RestartI2C(); IdleI2C();
  WriteI2C(LCD_ADDR | 0x00); IdleI2C();
  WriteI2C(0xf0 | LCD_BL | LCD_RW | mode); IdleI2C();
  StopI2C();
  return ((hc&0xf0) | ((lc>>4)&0x0f));
}

void LCDsend2x4(unsigned char c, unsigned char mode)
{
  unsigned char hc;
  unsigned char lc;
  
  hc = c & 0xf0;
  lc = (c << 4) & 0xf0;
    
  IdleI2C();
  StartI2C(); IdleI2C();
  WriteI2C(LCD_ADDR | 0x00); IdleI2C();
  WriteI2C(hc | LCD_BL | mode); IdleI2C();
  WriteI2C(hc | LCD_BL | LCD_EN | mode); IdleI2C();
  __delay_us(1);
  WriteI2C(hc | LCD_BL | mode); IdleI2C();
  WriteI2C(lc | LCD_BL | mode); IdleI2C();
  WriteI2C(lc | LCD_BL | LCD_EN | mode); IdleI2C();
  __delay_us(1);
  WriteI2C(lc | LCD_BL | mode); IdleI2C();
  StopI2C();    
  __delay_us(50);
}

void LCDinit(void)
{
    __delay_ms(50);
    LCDsend(0x30);
    LCDsend(0x34); __delay_us(500); LCDsend(0x30);
    __delay_ms(5);
    LCDsend(0x30);
    LCDsend(0x34); __delay_us(500); LCDsend(0x30);
    __delay_us(100);
    LCDsend(0x30);
    LCDsend(0x34); __delay_us(500); LCDsend(0x30);
    __delay_us(100);
    LCDsend(0x20);
    LCDsend(0x24); __delay_us(500); LCDsend(0x20);
    __delay_ms(5);

    LCDsend2x4(0x28, 0);
    LCDsend2x4(0x06, 0);
    LCDsend2x4(0x0f, 0);
    LCDsend2x4(0x01, 0);
    __delay_ms(2);
}

void LCDcmd(unsigned char c)
{
  LCDsend2x4(c, 0);
}

void LCDchar(unsigned char c)
{
  LCDsend2x4(c, LCD_RS);
}

void LCDstr(unsigned char *p)
{
  unsigned char c;
  
  while((c = *p++)) LCDchar(c);
}

int LCDbusy()
{
    if(LCDrecv(0) & 0x80) return 1;
    return 0;
}

struct Time {
    uint8_t h;
    uint8_t m;
    uint8_t s;
}; 

// Computes difference between time periods
void differenceBetweenTimePeriod(struct Time start,
                                 struct Time stop,
                                 struct Time *diff) {
   while (stop.s > start.s) {
      --start.m;
      start.s += 60;
   }
   diff->s = start.s - stop.s;
   while (stop.m > start.m) {
      --start.h;
      start.m += 60;
   }
   diff->m = start.m - stop.m;
   diff->h = start.h - stop.h;
}

void PWM_Output_D4_Enable (void){
    PWM6EN = 1;
}

void PWM_Output_D4_Disable (void){
    PWM6EN = 0;
}

int map(int x, int in_min, int in_max, int out_min, int out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

struct clockAlarm{
    struct Time alarmVal;
    bool trigger;
};

struct temperatureAlarm{
    unsigned char alarmTemp;
    bool trigger;
    bool triggered;
};

struct luminosityAlarm{
    unsigned char alarmLum;
    bool trigger;
    bool triggered;
};

struct Time t = {0,0,0}; // Time struct for current time

uint8_t temp;
uint8_t lumLevel;

struct clockAlarm clkAlarm;// Time struct for editing clock alarm
struct temperatureAlarm tempAlarm;
struct luminosityAlarm lumAlarm;

int dimingLed = 0;
struct Time alarmPWMStart = {0xff,0xff,0xff};

int editingClockAlarm = 0;
bool editingTempAlarm = false;
bool editingLumAlarm = false;

int mode = 0; //Mode of operation (0 no edit, 1 edit CLK, 2 edit Temp, 3 edit Lum, 4 toggle Alarm Enable/Disable)

void Clock_ISR(void) {    
    // Clock handler increment timer
    t.s++;
    
    if(t.s==60){
        t.m++;
        t.s=0;
        
        //Save variables to EEPROM
        DATAEE_WriteByte( EEAddr_INIT + (1 * 8), NREG); // NREG
        DATAEE_WriteByte( EEAddr_INIT + (2 * 8), PMON); // PMON
        DATAEE_WriteByte( EEAddr_INIT + (3 * 8), TALA); // TALA
        DATAEE_WriteByte( EEAddr_INIT + (4 * 8), clkAlarm.alarmVal.h); // ALAH
        DATAEE_WriteByte( EEAddr_INIT + (5 * 8), clkAlarm.alarmVal.m); // ALAM
        DATAEE_WriteByte( EEAddr_INIT + (6 * 8), clkAlarm.alarmVal.s); // ALAS
        DATAEE_WriteByte( EEAddr_INIT + (7 * 8), tempAlarm.alarmTemp); // ALAT
        DATAEE_WriteByte( EEAddr_INIT + (8 * 8), lumAlarm.alarmLum); // ALAL
        DATAEE_WriteByte( EEAddr_INIT + (9 * 8), ALAF); // ALAF
        DATAEE_WriteByte( EEAddr_INIT + (10 * 8), t.h); // CLKH
        DATAEE_WriteByte( EEAddr_INIT + (11 * 8), t.m); // CLKM
        DATAEE_WriteByte( EEAddr_INIT + (12 * 8), idx_RingBuffer); // Index of last ring buffer write
        DATAEE_WriteByte( EEAddr_INIT + (13 * 8), NREG + PMON + TALA + clkAlarm.alarmVal.h + clkAlarm.alarmVal.m + clkAlarm.alarmVal.s + tempAlarm.alarmTemp + lumAlarm.alarmLum + ALAF + t.h + t.m + idx_RingBuffer); // Check Sum
    }
    if(t.m==60){
        t.h++;
        t.m=0;
    }
    if(t.h==24){
        t.h=0;
    }
    
    //Check alarm trigger 
    if((ALAF == 'A') && t.s >= clkAlarm.alarmVal.s && t.m >= clkAlarm.alarmVal.m && t.h >= clkAlarm.alarmVal.h && editingClockAlarm == 0){
        alarmPWMStart.h = 0xff; //For the PWM LED to start
        clkAlarm.trigger = true;
        clkAlarm.alarmVal.h = 25; //Only triggered once until new val is given by user
    }
    
    LED_D5_Toggle();
}



void menuLCD_ISR(){
    char str[8];
    if(editingClockAlarm){
        sprintf(str, "%02d:%02d:%02d", clkAlarm.alarmVal.h,clkAlarm.alarmVal.m,clkAlarm.alarmVal.s);
    } else {
        sprintf(str, "%02d:%02d:%02d", t.h,t.m,t.s);
    }
    LCDcmd(0x80);
    LCDstr(str);
    
    //If alarms are enable
    if(ALAF == 'A'){
        LCDcmd(0x8F);
        LCDchar(ALAF);
        
        //If alarm is triggered by clock
        if(clkAlarm.trigger == true){
            LCDcmd(0x8B);
            LCDchar('C');
        } else{
            LCDcmd(0x8B);
            LCDchar(' ');
        }

        //If alarm is triggered by temperature 
        if(tempAlarm.trigger == true){
            LCDcmd(0x8C);
            LCDchar('T');
        } else{
            LCDcmd(0x8C);
            LCDchar(' ');
        }

        //If alarm is triggered by luminosity 
        if(lumAlarm.trigger == true){
            LCDcmd(0x8D);
            LCDchar('L');
        } else{
            LCDcmd(0x8D);
            LCDchar(' ');
        }
        if(clkAlarm.trigger || tempAlarm.trigger || lumAlarm.trigger){
            if(alarmPWMStart.h == 0xff){
                alarmPWMStart.h = t.h;
                alarmPWMStart.m = t.m;
                alarmPWMStart.s = t.s;
            }
            struct Time diff = {0,0,0};
            differenceBetweenTimePeriod( t, alarmPWMStart, &diff);
            
            //PWM AINDA ESTA MAL
            if(diff.s <= TALA){
                if(PWM6EN==0){ //Verifica se o Timer2 e PWM esta desligado
                    TMR2_StartTimer();
                    PWM_Output_D4_Enable();
                }
                if(dimingLed <= 1023){
                    dimingLed += 200;
                } else{
                    dimingLed = 0;
                }
                PWM6_LoadDutyValue(dimingLed);
            } else if(PWM6EN==1){ //Verifica se o Timer2 e PWM esta ligado
                PWM6_LoadDutyValue(0);
                TMR2_StopTimer();
                PWM_Output_D4_Disable();
            }
        }
    } else{ //If alarms are disabled
        LCDcmd(0x8F);
        LCDchar(ALAF);
    }
    
    LCDcmd(0xc0);
    char tt[4];
    if(editingTempAlarm){
        sprintf(tt, "%02d C", tempAlarm.alarmTemp);
    } else{
        sprintf(tt, "%02d C", temp);
    }
	LCDstr(tt);
    
    LCDcmd(0xcd);
    char l[3];
    
    if(editingLumAlarm){
        sprintf(l, "L %d", lumAlarm.alarmLum);
    } else{
        sprintf(l, "L %d", lumLevel);
    }
    LCDstr(l);
    
    if(mode == 1){
        if(editingClockAlarm == 1){
            LCDcmd(0x81);
        } else if(editingClockAlarm == 2){
            LCDcmd(0x84);
        } else if(editingClockAlarm == 3){
            LCDcmd(0x87);
        }
    } else if(mode == 2){
        LCDcmd(0xc1);
    } else if(mode == 3){
        LCDcmd(0xcf);
    } else if(mode == 4){
        LCDcmd(0x8f);
    }
}

int prevTemp = -1;
int prevLumLevel = -1;

void monitoring_ISR(){
    temp = (uint8_t)tsttc(); //Get temp
    
    lumLevel = ADCC_GetSingleConversion(channel_ANA0) >> 13; //Shift of 13 to get only 3 most significant bits = 8 levels
    
    if(prevTemp != temp || prevLumLevel != lumLevel){ //If Different values saving in eeprom
        
        DATAEE_WriteByte( (idx_RingBuffer * 0x5) + EEAddr_RING + (8*0) , t.h);
        DATAEE_WriteByte( (idx_RingBuffer * 0x5) + EEAddr_RING + (8*1) , t.m);
        DATAEE_WriteByte( (idx_RingBuffer * 0x5) + EEAddr_RING + (8*2) , t.s);
        DATAEE_WriteByte( (idx_RingBuffer * 0x5) + EEAddr_RING + (8*3) , temp);
        DATAEE_WriteByte( (idx_RingBuffer * 0x5) + EEAddr_RING + (8*4) , lumLevel);
        
        idx_RingBuffer++;
        if(idx_RingBuffer > NREG){
            idx_RingBuffer = 0;
        }
        prevTemp = temp;
        prevLumLevel = lumLevel;  
    }
    
    
    if(ALAF == 'A'){
        //Check luminosity alarm trigger
        if((lumAlarm.alarmLum > lumLevel) && (editingLumAlarm == false)){
            if(!lumAlarm.triggered){
                alarmPWMStart.h = 0xff;//For the PWM LED to start
            }
            lumAlarm.triggered = true;
            
            lumAlarm.trigger = true;
            LED_D2_SetHigh();
        } else{
            lumAlarm.triggered = false;
            LED_D2_SetLow();
        }

        //Check temperature alarm trigger
        if((tempAlarm.alarmTemp < temp) && (editingTempAlarm == false)){
            if(!tempAlarm.triggered){
                alarmPWMStart.h = 0xff;//For the PWM LED to start
            }
            
            tempAlarm.triggered = true;
            
            tempAlarm.trigger = true;
            LED_D3_SetHigh();
        } else {
            
            tempAlarm.triggered = false;
            LED_D3_SetLow();
        }
    }
}

void editClock(){
    
    while(1){
        /*if(S1_GetValue() == LOW){
            __delay_ms(50);
            editingClockAlarm++;
            while(S1_GetValue()==LOW){}; //Waiting for user to stop pressing S1
            if(editingClockAlarm > 3){
                editingClockAlarm = 0;
                mode = 2;
                break;
            }
        }*/
        
        if(S2_GetValue() == LOW){ //Switch 2 pressed
            if(editingClockAlarm == 1){ //Editing Hours
                if(clkAlarm.alarmVal.h >= 23){
                    clkAlarm.alarmVal.h = 0;
                } else{
                    clkAlarm.alarmVal.h++; 
                }
            } else if(editingClockAlarm == 2){//Editing Minutes
                if(clkAlarm.alarmVal.m == 59){
                    clkAlarm.alarmVal.m = 0;
                } else{
                    clkAlarm.alarmVal.m++; 
                }
            } else if(editingClockAlarm == 3){//Editing Seconds
                if(clkAlarm.alarmVal.s == 59){
                    clkAlarm.alarmVal.s = 0;
                } else{
                    clkAlarm.alarmVal.s++; 
                }
            }
            __delay_ms(S2DELAY);
        }
        if(mode != 1){
            editingClockAlarm = 0;
            break;
        }
    }
}

void editTemp(){
    editingTempAlarm = true;
        
    while(1){
        /*if(S1_GetValue() == LOW){
            __delay_ms(50);
            editingTempAlarm = false;
            mode = 3;
            while(S1_GetValue()==LOW){}; //Waiting for user to stop pressing S1
            break;
        }*/
        
        if(S2_GetValue() == LOW){ //Switch 2 pressed
            tempAlarm.alarmTemp++;
            if(tempAlarm.alarmTemp > 50){
                tempAlarm.alarmTemp = 0;
            }
            __delay_ms(S2DELAY);
        }
        if(mode != 2){
            editingTempAlarm = false;
            break;
        }
    }    
}

void editLum(){
    editingLumAlarm = true;
    
    while(1){
        /*if(S1_GetValue() == LOW){
            __delay_ms(50);
            editingLumAlarm = false;
            mode = 4;
            while(S1_GetValue()==LOW){}; //Waiting for user to stop pressing S1
            break;
        }*/
        
        if(S2_GetValue() == LOW){ //Switch 2 pressed
            lumAlarm.alarmLum++;
            if(lumAlarm.alarmLum > 7){ //Se o user meter 0 nunca vai passar abaixo desse valor o que significa que o alarme nao dispara
                lumAlarm.alarmLum = 0;
            }
            __delay_ms(S2DELAY);
        }
        if(mode != 3){
            editingLumAlarm = false;
            break;
        }
    }  
}

void toggleAlarms(){
    
    while(1){
        /*if(S1_GetValue() == LOW){
            __delay_ms(50);
            mode = 0;
            while(S1_GetValue()==LOW){}; //Waiting for user to stop pressing S1
            break;
        }*/
        
        if(S2_GetValue() == LOW){ //Switch 2 pressed
            if(ALAF == 'A'){
                ALAF = 'a';
            } else {
                ALAF = 'A';
            }
            __delay_ms(S2DELAY);
        }
        if(mode != 4){
            mode = 0;
            break;
        }
    }
}

void S1_ISR(){
    __delay_ms(50);
    
    // First clears LCD and only switches mode when S1 is pressed again and theirs no alarms
    if(mode == 0 && (clkAlarm.trigger || tempAlarm.trigger || lumAlarm.trigger)){
        clkAlarm.trigger = false;
        tempAlarm.trigger = false;
        lumAlarm.trigger = false;
    } else{
        if(mode == 1){
            editingClockAlarm++;
            if(editingClockAlarm > 3){
                editingClockAlarm = 0;
                mode++;
            }
        } else{
            mode++;
        }
    }
}

void main(void)
{
    // initialize the device
    SYSTEM_Initialize();
    
    PWM6_LoadDutyValue(0);
    TMR2_StopTimer();
    PWM_Output_D4_Disable();
    
    TMR1_SetInterruptHandler(Clock_ISR);
    
    TMR3_SetInterruptHandler(menuLCD_ISR);
    
    TMR5_SetInterruptHandler(monitoring_ISR);
    
    INT_SetInterruptHandler(S1_ISR);

    uint8_t checkSumAux = 0;
    bool notInit = true;
    bool corrupted = false;
    if(DATAEE_ReadByte(EEAddr_INIT) == 'S'){
        notInit = false;
        for(int i = 1; i < 13; i++){
            checkSumAux += DATAEE_ReadByte(EEAddr_INIT + (i*8));
        }
        if(checkSumAux != DATAEE_ReadByte(EEAddr_INIT + (13*8))){
            corrupted = true;
        }
    }
    
    if(notInit || corrupted){
        DATAEE_WriteByte( EEAddr_INIT + (0 * 8), 'S'); //Magic word if = S means that the program ran at least once
        DATAEE_WriteByte( EEAddr_INIT + (1 * 8), 25); // NREG
        DATAEE_WriteByte( EEAddr_INIT + (2 * 8), 3); // PMON
        DATAEE_WriteByte( EEAddr_INIT + (3 * 8), 5); // TALA
        DATAEE_WriteByte( EEAddr_INIT + (4 * 8), 12); // ALAH
        DATAEE_WriteByte( EEAddr_INIT + (5 * 8), 0); // ALAM
        DATAEE_WriteByte( EEAddr_INIT + (6 * 8), 0); // ALAS
        DATAEE_WriteByte( EEAddr_INIT + (7 * 8), 28); // ALAT
        DATAEE_WriteByte( EEAddr_INIT + (8 * 8), 4); // ALAL
        DATAEE_WriteByte( EEAddr_INIT + (9 * 8), 'a'); // ALAF
        DATAEE_WriteByte( EEAddr_INIT + (10 * 8), 0); // CLKH
        DATAEE_WriteByte( EEAddr_INIT + (11 * 8), 0); // CLKM
        DATAEE_WriteByte( EEAddr_INIT + (12 * 8), 0); // Index of last ring buffer write
        DATAEE_WriteByte( EEAddr_INIT + (13 * 8), 174); // Check Sum
    }
    
    NREG = DATAEE_ReadByte(EEAddr_INIT + (1*8));
    PMON = DATAEE_ReadByte(EEAddr_INIT + (2*8));
    TALA = DATAEE_ReadByte(EEAddr_INIT + (3*8));
    clkAlarm.alarmVal.h = DATAEE_ReadByte(EEAddr_INIT + (4*8)); //ALAH
    clkAlarm.alarmVal.m = DATAEE_ReadByte(EEAddr_INIT + (5*8)); //ALAM
    clkAlarm.alarmVal.s = DATAEE_ReadByte(EEAddr_INIT + (6*8)); //ALAS
    tempAlarm.alarmTemp = DATAEE_ReadByte(EEAddr_INIT + (7*8)); //ALAT
    lumAlarm.alarmLum = DATAEE_ReadByte(EEAddr_INIT + (8*8)); //ALAL
    ALAF = DATAEE_ReadByte(EEAddr_INIT + (9*8));
    t.h = DATAEE_ReadByte(EEAddr_INIT + (10*8));
    t.m = DATAEE_ReadByte(EEAddr_INIT + (11*8));
    idx_RingBuffer = DATAEE_ReadByte(EEAddr_INIT + (12*8));
    
    
    //Init struct, needs improvements
    tempAlarm.trigger = false;
    tempAlarm.triggered = false;
    
    lumAlarm.trigger = false;
    lumAlarm.triggered = false;
    
    clkAlarm.trigger = false;
    
    
    i2c1_driver_open();
    I2C_SCL = 1;
    I2C_SDA = 1;
    WPUC3 = 1;
    WPUC4 = 1;
    LCDinit();

    // Enable the Global Interrupts
    INTERRUPT_GlobalInterruptEnable();

    // Enable the Peripheral Interrupts
    INTERRUPT_PeripheralInterruptEnable();
    
    SLEEP();

    while (1)
    {
        switch(mode){
                case 0: SLEEP();
                case 1: 
                    editClock(); //Clock Edit Handler
                case 2:
                    editTemp(); //Temperature Edit Handler
                case 3:
                    editLum(); //Luminosity Edit Handler
                case 4:
                    toggleAlarms(); //Enables/Disables Alarms
            }
    }   
}
/**
 End of File
*/
