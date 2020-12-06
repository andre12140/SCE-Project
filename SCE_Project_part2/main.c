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
#include "stdlib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>



#define S2DELAY 400 //Use to define time to wait for debounce
#define S1DELAY 50  //Use to define time to wait for debounce

#define EEAddr_INIT    0x7000        // EEPROM starting address (Used for storing init data)
#define EEAddr_RING    0x7014        // Ring Buffer starting address
//If NREG = 25 Last address = 0x7095 and Last address of eeprom is 0x70FF

#define LCD_ADDR 0x4e   // 0x27 << 1
#define LCD_BL 0x08
#define LCD_EN 0x04
#define LCD_RW 0x02
#define LCD_RS 0x01

#define SOM 0xFD /* start of message */
#define EOM 0xFE /* end of message */
#define RCLK 0xC0 /* read clock */
#define SCLK 0XC1 /* set clock */
#define RTL 0XC2 /* read temperature and luminosity */
#define RPAR 0XC3 /* read parameters */
#define MMP 0XC4 /* modify monitoring period */
#define MTA 0XC5 /* modify time alarm */
#define RALA 0XC6 /* read alarms (clock, temperature, luminosity, active/inactive) */
#define DAC 0XC7 /* define alarm clock */
#define DATL 0XC8 /* define alarm temperature and luminosity */
#define AALA 0XC9 /* activate/deactivate alarms */
#define IREG 0XCA /* information about registers (NREG, nr, iread, iwrite)*/
#define TRGC 0XCB /* transfer registers (curr. position)*/
#define TRGI 0XCC /* transfer registers (index) */
#define NMFL 0XCD /* notification memory (half) full */
#define CMD_OK 0 /* command successful */
#define CMD_ERROR 0xFF /* error in command */

uint8_t NREG = 25;   //Number of registers, Max value is 46 to still be in the eeprom
uint8_t PMON = 3;   //Monitoring period
uint8_t TALA = 5;   //Duration of alarm signal (PWM)
/*uint8_t ALAH = 0;   //Hours of alarm clock
uint8_t ALAM = 0;   //Minutes of alarm clock
uint8_t ALAS = 0;   //Seconds of alarm clock
uint8_t ALAT = 0;   //Alarm threshold for Temperature
uint8_t ALAL = 0;*/   //Alarm threshold for Luminosity
uint8_t ALAF = 0;   //Alarm Flag (Initially disabled)
/*uint8_t CLKH = 0;   //Initial value for clock hours
uint8_t CLKM = 0;*/   //Initial value for clock minutes
//uint8_t idx_RingBuffer = 0; //Index of Ring Buffer to EEPROM

void cmd_rc(int, char *);
void cmd_sc(int, char *);
void cmd_rtl(int, char *);
void cmd_rp(int, char *);
void cmd_mmp(int, char *);
void cmd_mta(int, char *);
void cmd_ra(int, char *);
void cmd_dac(int, char *);
void cmd_dtl(int, char *);
void cmd_aa(int, char *);
void cmd_ir(int, char *);
void cmd_trc(int, char *);
void cmd_tri(int, char *);

struct command_d
{
  void (*cmd_fnct)(int, char *);
  char cmd_name;
} const commands[] = {
    {cmd_rc, RCLK},
    {cmd_sc, SCLK},
    {cmd_rtl, RTL},
    {cmd_rp, RPAR},
    {cmd_mmp, MMP},
    {cmd_mta, MTA},
    {cmd_ra, RALA},
    {cmd_dac, DAC},
    {cmd_dtl, DATL},
    {cmd_aa, AALA},
    {cmd_ir, IREG},
    {cmd_trc, TRGC},
    {cmd_tri, TRGI},
};

struct Time {
    uint8_t h;
    uint8_t m;
    uint8_t s;
}; 

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

#define NCOMMANDS (sizeof(commands) / sizeof(struct command_d))

bool S1_Value = false;

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

int modeFlag = 0; //Mode of operation (0 no edit, 1 edit CLK, 2 edit Temp, 3 edit Lum, 4 toggle Alarm Enable/Disable)

bool PWM_on=false;

int prevTemp = -1;
int prevLumLevel = -1;

bool updateLCD = true;
bool flagS1pushed = false;

uint8_t iread = 0;
uint8_t iwrite = 0;
uint8_t nr = 0;

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
        DATAEE_WriteByte( EEAddr_INIT + (12 * 8), iwrite); // Index of last ring buffer write
        DATAEE_WriteByte( EEAddr_INIT + (13 * 8), NREG + PMON + TALA + clkAlarm.alarmVal.h + clkAlarm.alarmVal.m + clkAlarm.alarmVal.s + tempAlarm.alarmTemp + lumAlarm.alarmLum + ALAF + t.h + t.m + iwrite); // Check Sum
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
    updateLCD = true;
}

//Se fizer update muito rapido o LCD para de funcionar corretamente?
void update_menuLCD(){
    
    char str[8];
    if(editingClockAlarm){
        sprintf(str, "%02d:%02d:%02d", clkAlarm.alarmVal.h,clkAlarm.alarmVal.m,clkAlarm.alarmVal.s);
    } else {
        sprintf(str, "%02d:%02d:%02d", t.h,t.m,t.s);
    }
    LCDcmd(0x80);
    LCDstr(str);
    
    //If alarm is triggered by clock
    if(clkAlarm.trigger == true){
        LCDcmd(0x8B);
        LCDchar('C');
    } else if(modeFlag == 0){
        LCDcmd(0x8B);
        LCDchar(' ');
    }

    //If alarm is triggered by temperature 
    if(tempAlarm.trigger == true){
        LCDcmd(0x8C);
        LCDchar('T');
    } else if(modeFlag == 0){
        LCDcmd(0x8C);
        LCDchar(' ');
    }

    //If alarm is triggered by luminosity 
    if(lumAlarm.trigger == true){
        LCDcmd(0x8D);
        LCDchar('L');
    } else if(modeFlag == 0){
        LCDcmd(0x8D);
        LCDchar(' ');
    }
    
    LCDcmd(0x8F);
    LCDchar(ALAF);
    
    //If alarms are enable
    if(ALAF == 'A'){
        if(clkAlarm.trigger || tempAlarm.trigger || lumAlarm.trigger){
            if(alarmPWMStart.h == 0xff){
                alarmPWMStart.h = t.h;
                alarmPWMStart.m = t.m;
                alarmPWMStart.s = t.s;
            }
            struct Time diff = {0,0,0};
            differenceBetweenTimePeriod( t, alarmPWMStart, &diff);
            
            if(diff.s <= TALA){
                PWM_on = true;
                /*if(PWM6EN==0){ //Verifica se o Timer2 e PWM esta desligado
                    TMR2_StartTimer();
                    PWM_Output_D4_Enable();
                }
                if(dimingLed <= 330){ //max 1023 mas como nao se nota muita diferenca para valores altos meteu se mais baixo
                    dimingLed += 30;
                } else{
                    dimingLed = 0;
                }
                PWM6_LoadDutyValue(dimingLed);*/
            } else if(PWM6EN==1){ //Verifica se o Timer2 e PWM esta ligado
                PWM_on = false;
                PWM6_LoadDutyValue(0);
                TMR2_StopTimer();
                PWM_Output_D4_Disable();
            }
        } else if(PWM6EN==1){ //Verifica se o Timer2 e PWM esta ligado
                PWM_on = false;
                PWM6_LoadDutyValue(0);
                TMR2_StopTimer();
                PWM_Output_D4_Disable();
        }
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
    
    if(modeFlag != 0){
        LCDcmd(0x8B);
        LCDstr("CTL");
    }
    
    if(modeFlag == 1){
        if(editingClockAlarm == 0){
            LCDcmd(0x8B);
        } else{
            if(editingClockAlarm == 1){
            LCDcmd(0x81);
            } else if(editingClockAlarm == 2){
                LCDcmd(0x84);
            } else if(editingClockAlarm == 3){
                LCDcmd(0x87);
            }
        }
    } else if(modeFlag == 2){
        
        if(editingTempAlarm == false){
            LCDcmd(0x8c);
        }else {
            LCDcmd(0xc1);
        }
        
    } else if(modeFlag == 3){
        
        if(editingLumAlarm == false){
            LCDcmd(0x8d);
        }else {
            LCDcmd(0xcf);
        }
        
    } else if(modeFlag == 4){
        LCDcmd(0x8f);
    }
}

bool flagNr = false; //It means that number of saved readings is lower than NREG

//Chamado com um periodo de PMON, Não esta a funcionar?
void monitoring_ISR(){
    temp = (uint8_t)tsttc(); //Get temp
    
    lumLevel = ADCC_GetSingleConversion(channel_ANA0) >> 13; //Shift of 13 to get only 3 most significant bits = 8 levels
    
    if(prevTemp != temp || prevLumLevel != lumLevel){ //If Different values saving in eeprom
        
        /*DATAEE_WriteByte( (iwrite * 0x5) + EEAddr_RING + (8*0) , t.h);
        DATAEE_WriteByte( (iwrite * 0x5) + EEAddr_RING + (8*1) , t.m);
        DATAEE_WriteByte( (iwrite * 0x5) + EEAddr_RING + (8*2) , t.s);
        DATAEE_WriteByte( (iwrite * 0x5) + EEAddr_RING + (8*3) , temp);
        DATAEE_WriteByte( (iwrite * 0x5) + EEAddr_RING + (8*4) , lumLevel);*/
        DATAEE_WriteByte( (iwrite * 0x5) + EEAddr_RING + 0x0 , t.h);
        DATAEE_WriteByte( (iwrite * 0x5) + EEAddr_RING + 0x1 , t.m);
        DATAEE_WriteByte( (iwrite * 0x5) + EEAddr_RING + 0x2 , t.s);
        DATAEE_WriteByte( (iwrite * 0x5) + EEAddr_RING + 0x3 , temp);
        DATAEE_WriteByte( (iwrite * 0x5) + EEAddr_RING + 0x4 , lumLevel);
        
        if((nr == NREG) && (iread == iwrite)){
            iread++;
        }
        
        iwrite++;
        if(iwrite > NREG - 1){
            flagNr = true;
            iwrite = 0;
        }
        if(flagNr){
            nr = NREG;
        } else{
            nr++;
        }
        
        if(iread > NREG-1){
            iread = 0;
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

void S1button(){
    // First clears LCD and only switches mode when S1 is pressed again and theirs no alarms
    if(modeFlag == 0 && (clkAlarm.trigger || tempAlarm.trigger || lumAlarm.trigger)){
        clkAlarm.trigger = false;
        tempAlarm.trigger = false;
        lumAlarm.trigger = false;
        updateLCD = true;
    } else{
        if(modeFlag == 1){
            if(editingClockAlarm >= 1){
                editingClockAlarm++;
            }
            if(editingClockAlarm > 3){
                editingClockAlarm = 0;
            }
        }
        if(editingClockAlarm == 0){ //Verify if not editing clock otherwise always increment mode
            modeFlag++;
        }
    }
    __delay_ms(S1DELAY);
}

void editClock(){
    
    while(1){
        if(flagS1pushed){
            S1button();
            flagS1pushed=false;
            /*update_menuLCD();
            updateLCD=false;
            __delay_ms(100); //se der update do LCD muito rapido pode comecar a escrever lixo*/
        }
        if(S2_GetValue() == LOW){ //Switch 2 pressed
            if(editingClockAlarm == 0){
                editingClockAlarm = 1; //Started Editing clock
                
            } else if(editingClockAlarm == 1){ //Editing Hours
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
            update_menuLCD();
        }
        if(modeFlag != 1){
            editingClockAlarm = 0;
            break;
        }
        if(updateLCD){
            update_menuLCD();
            updateLCD=false;
        }
    }
}

void editTemp(){
    while(1){
        if(flagS1pushed){
            S1button();
            flagS1pushed=false;
            /*update_menuLCD();
            updateLCD=false;
            __delay_ms(100); //se der update do LCD muito rapido pode comecar a escrever lixo*/
        }
        if(S2_GetValue() == LOW){ //Switch 2 pressed
            if(editingTempAlarm == false){
                editingTempAlarm = true;
            } else {
                tempAlarm.alarmTemp++;
                if(tempAlarm.alarmTemp > 50){
                    tempAlarm.alarmTemp = 0;
                }
            }
            __delay_ms(S2DELAY);
            update_menuLCD();
        }
        if(modeFlag != 2){
            editingTempAlarm = false;
            break;
        }
        if(updateLCD){
            update_menuLCD();
            updateLCD=false;
        }
    }    
}

void editLum(){
    while(1){
        if(flagS1pushed){
            S1button();
            flagS1pushed=false;
            /*update_menuLCD();
            updateLCD=false;
            __delay_ms(100); //se der update do LCD muito rapido pode comecar a escrever lixo*/
        }
        if(S2_GetValue() == LOW){ //Switch 2 pressed
            if(editingLumAlarm == false){ 
                editingLumAlarm = true;
            } else {
                lumAlarm.alarmLum++;
                if(lumAlarm.alarmLum > 7){ //Se o user meter 0 nunca vai passar abaixo desse valor o que significa que o alarme nao dispara
                    lumAlarm.alarmLum = 0;
                }
            }
            __delay_ms(S2DELAY);
            update_menuLCD();
        }
        if(modeFlag != 3){
            editingLumAlarm = false;
            break;
        }
        if(updateLCD){
            update_menuLCD();
            updateLCD=false;
        }
    }
}

void toggleAlarms(){
    
    while(1){
        if(flagS1pushed){
            S1button();
            flagS1pushed=false;
            /*update_menuLCD();
            updateLCD=false;
            __delay_ms(100); //se der update do LCD muito rapido pode comecar a escrever lixo*/
        }
        if(S2_GetValue() == LOW){ //Switch 2 pressed
            if(ALAF == 'A'){
                ALAF = 'a';
            } else {
                ALAF = 'A';
            }
            __delay_ms(S2DELAY);
            update_menuLCD();
        }
        if(modeFlag != 4){
            modeFlag = 0;
            break;
        }
        if(updateLCD){
            update_menuLCD();
            updateLCD=false;
        }
    }
}

void S1_ISR(){
    PIE0bits.INTE = 0; //Disables external ISR
    flagS1pushed = true;
    /*if(modeFlag == 0 && (clkAlarm.trigger || tempAlarm.trigger || lumAlarm.trigger)){
        clkAlarm.trigger = false;
        tempAlarm.trigger = false;
        lumAlarm.trigger = false;
        updateLCD = true;
    } else{
        if(modeFlag == 1){
            if(editingClockAlarm >= 1){
                editingClockAlarm++;
            }
            if(editingClockAlarm > 3){
                editingClockAlarm = 0;
            }
        }
        if(editingClockAlarm == 0){ //Verify if not editing clock otherwise always increment mode
            modeFlag++;
        }
    }*/
    EXT_INT_InterruptFlagClear();
    PIE0bits.INTE = 1; //Enables external ISR
}

/*-------------------------------------------------------------
 |                      eCos Commands                         |
 -------------------------------------------------------------*/

void sendMessage(int num, char *buffer){
    int n = 0;
    while(n<num){
        putch(buffer[n]);
        n++;
    }
}

void sendOKMessage(uint8_t cmd){
    uint8_t bufw[4];
    bufw[0] = (uint8_t)SOM;
    bufw[1] = (uint8_t)cmd;
    bufw[2] = (uint8_t)CMD_OK;
    bufw[3] = (uint8_t)EOM;
    
    sendMessage(4,bufw);
}

void sendERRORMessage(uint8_t cmd){
    uint8_t bufw[4];
    bufw[0] = (uint8_t)SOM;
    bufw[1] = (uint8_t)cmd;
    bufw[2] = (uint8_t)CMD_ERROR;
    bufw[3] = (uint8_t)EOM;
    
    sendMessage(4,bufw);
}

void cmd_rc(int num, char *buffer){
    uint8_t buff[6];
    buff[0] = (uint8_t)SOM;
    buff[1] = (uint8_t)RCLK;
    buff[2] = t.h;
    buff[3] = t.m;
    buff[4] = t.s;
    buff[5] = (uint8_t)EOM;
    
    sendMessage(6,buff);
}

void cmd_sc(int num, char *buffer){
    uint8_t h = buffer[2];
    uint8_t m = buffer[3];
    uint8_t s = buffer[4];
    if((h >= 0 && h < 24) && (m >= 0 && m < 60) && (s >= 0 && s < 60) && num == 6){
        t.h = h;
        t.m = m;
        t.s = s;
        sendOKMessage((uint8_t)SCLK);
    } else {
        sendERRORMessage((uint8_t)SCLK);
    }

}

void cmd_rtl(int num, char *buffer){
    uint8_t buff[5];
    buff[0] = (uint8_t)SOM;
    buff[1] = (uint8_t)RTL;
    buff[2] = temp;
    buff[3] = lumLevel;
    buff[4] = (uint8_t)EOM;
    
    sendMessage(5,buff);
}

void cmd_rp(int num, char *buffer){
    uint8_t buff[5];
    buff[0] = (uint8_t)SOM;
    buff[1] = (uint8_t)RPAR;
    buff[2] = PMON;
    buff[3] = TALA;
    buff[4] = (uint8_t)EOM;
    
    sendMessage(5,buff);
}

// VERIFICAR, PENSO QUE NAO FUNCIONA
void cmd_mmp(int num, char *buffer){
    
    if(buffer[2] == 0x0){
        TMR5_StopTimer();
    } else if(buffer[2] >= 0x01 && buffer[2] <= 0x10){
        PMON = buffer[2];
        //Min PMON = 1 s
        //Max PMON = 16 s
        //LFINT = 31000;//31kHz
        //Prescaler = 8;
        //max_value = 65536; //(16 bits)
        uint16_t timerValue = (uint32_t)65536 - (uint32_t)((uint32_t)((uint32_t)PMON*(uint32_t)31000)/8);
        setTimer5ReloadVal(timerValue);
        TMR5_StartTimer();
    } else {
        sendERRORMessage((uint8_t)MMP);
    }
    sendOKMessage((uint8_t)MMP);
}

void cmd_mta(int num, char *buffer){
    if(buffer[2] >= 0x00 && buffer[2] < 0x3c){
        TALA = buffer[2];
        sendOKMessage((uint8_t)MTA);
    } else {
        sendERRORMessage((uint8_t)MTA);
    }
}

void cmd_ra(int num, char *buffer){
    uint8_t buff[9];
    buff[0] = (uint8_t)SOM;
    buff[1] = (uint8_t)RALA;
    buff[2] = clkAlarm.alarmVal.h;
    buff[3] = clkAlarm.alarmVal.m;
    buff[4] = clkAlarm.alarmVal.s;
    buff[5] = tempAlarm.alarmTemp;
    buff[6] = lumAlarm.alarmLum;
    buff[7] = ALAF == 'A' ? 1 : 0;
    buff[8] = (uint8_t)EOM;
    
    sendMessage(9,buff);
}

void cmd_dac(int num, char *buffer){
    uint8_t h = buffer[2];
    uint8_t m = buffer[3];
    uint8_t s = buffer[4];
    if((h >= 0 && h < 24) && (m >= 0 && m < 60) && (s >= 0 && s < 60) && num == 6){
        clkAlarm.alarmVal.h = h;
        clkAlarm.alarmVal.m = m;
        clkAlarm.alarmVal.s = s;
        sendOKMessage((uint8_t)DAC);
    } else {
        sendERRORMessage((uint8_t)DAC);
    }
}

void cmd_dtl(int num, char *buffer){
    uint8_t tempAux = buffer[2];
    uint8_t lumAux = buffer[3];
    if((tempAux >= 0 && tempAux < 50) && (lumAux >= 0 && lumAux < 8) && num == 5){
        tempAlarm.alarmTemp = buffer[2];
        lumAlarm.alarmLum = buffer[3];
        sendOKMessage((uint8_t)DATL);
    } else {
        sendERRORMessage((uint8_t)DATL);
    }
}

void cmd_aa(int num, char *buffer){
    if(buffer[2] == 0 && num == 4){
        ALAF = 'a';
        sendOKMessage((uint8_t)AALA);
    } else if(buffer[2] == 1 && num == 4){
        ALAF = 'A';
        sendOKMessage((uint8_t)AALA);
    } else {
        sendERRORMessage((uint8_t)AALA);
    }
}

void cmd_ir(int num, char *buffer){
    uint8_t buff[7];
    buff[0] = (uint8_t)SOM;
    buff[1] = (uint8_t)IREG;
    buff[2] = NREG;
    buff[3] = nr;
    buff[4] = iread;
    buff[5] = iwrite;
    buff[6] = (uint8_t)EOM;
    
    sendMessage(7,buff);
}

//Registo de vez enquanto vêm errados
void cmd_trc(int num, char *buffer){
    if(num == 4){
        int n = buffer[2];
        int aux = (iwrite-1-iread);
        if(aux < 0){
            aux = iwrite + 1 + (NREG - iread);
        }
        if((n > nr) || (n > aux)){
            sendERRORMessage((uint8_t)TRGC);
            return;
        }
        //uint8_t sizeofMessage = 5*n + 3;
        //uint8_t buff[5*(10-1) + 3]; //Numero máximo de registos
        //uint8_t *buff; //Numero máximo de registos
        //buff = malloc(sizeofMessage);
        uint8_t buffInit[3];
        buffInit[0] = (uint8_t)SOM;
        buffInit[1] = (uint8_t)TRGC;
        buffInit[2] = (uint8_t)n;
        sendMessage(3,buffInit);
        int i;
        uint8_t j;
        uint8_t buffData[5];
        uint16_t address = 0;
        for(i = 0; i < n; i++){ //Read n registers
            for(j = 0; j < 5; j++){ //Read one register parameters (5 bytes)
                address =  (iread * 0x5) + EEAddr_RING + j;
                buffData[j] = DATAEE_ReadByte(address);
            }
            sendMessage(5,buffData);
            iread++;
            if(iread>NREG-1){
                iread=0;
            }
        }
        uint8_t buffEOM[1];
        buffEOM[0] = (uint8_t)EOM;
        sendMessage(1,buffEOM);
    } else{
        sendERRORMessage((uint8_t)TRGC);
    }
}
//READBYTE esta mal, nao posso simplesmente ir subtraindo i que posso sair fora do array
//Os registos que lemos nao se leem outra vez?

//Ler do indice ate iwrite ou do indice ate iread?
void cmd_tri(int num, char *buffer){
    if(num == 5){
        uint8_t n = buffer[2];
        uint8_t index = buffer[3];
        
        if((n > nr) || (index < 0) || (index > NREG-1)){
            sendERRORMessage((uint8_t)TRGI);
            return;
        }
        
        uint8_t startingIndex;
        if(((iwrite-1)-index) < 0){
            startingIndex = NREG - ((iwrite-1)-index);
        } else {
            startingIndex = ((iwrite-1)-index);
        }
        
        //Send starting message
        uint8_t buffInit[4];
        buffInit[0] = (uint8_t)SOM;
        buffInit[1] = (uint8_t)TRGC;
        buffInit[2] = (uint8_t)n;
        buffInit[2] = (uint8_t)index;
        sendMessage(4,buffInit);
        
        //Send data from index specified until iwrite
        int nMessagesSent = 0;
        int i = startingIndex;
        int indexAux = index;
        uint8_t j;
        uint8_t buffData[5];
        while(indexAux){
            for(j = 0; j < 5; j++){ //Read one register parameters (5 bytes)
                buffData[j] = DATAEE_ReadByte( (i * 0x5) + EEAddr_RING + j);
            }
            sendMessage(5,buffData);
            i++;
            indexAux--;
            if(i >= NREG){
                i=0;
            }
        }
        
        //Send End of Message
        uint8_t buffEOM[1];
        buffEOM[0] = (uint8_t)EOM;
        sendMessage(1,buffEOM);
    } else{
        sendERRORMessage((uint8_t)TRGI);
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
    iwrite = DATAEE_ReadByte(EEAddr_INIT + (12*8));
    
    
    //Init struct
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
    
    //SLEEP();
    
    uint8_t c;
    char buff[20] = "";
    int n=0;
    int i=0;
    
    while (1)
    {

        /*if(EUSART_is_rx_ready()){
            while(c != (uint8_t)EOM){
                c = getch();
                if((c == (uint8_t)SOM || buff[0] == (uint8_t)SOM)){
                    if(c == (uint8_t)SOM){
                        memset(buff, 0, sizeof buff); //Clean Array
                        n=0;
                    }
                    buff[n] = c;
                    n++;
                    if(n == 20){
                        c = 0x01;
                        memset(buff, 0, sizeof buff); //Clean Array
                        n=0;
                        break;
                    }
                }
            }
        }*/
        
        while(EUSART_is_rx_ready()){
            c = getch();
            if((c == (uint8_t)SOM || buff[0] == (uint8_t)SOM)){
                if(c == (uint8_t)SOM){
                    memset(buff, 0, sizeof buff); //Clean Array
                    n=0;
                }
                buff[n] = c;
                n++;
                if(n == 20){
                    memset(buff, 0, sizeof buff); //Clean Array
                    n=0;
                }
            }
            if(c == (uint8_t)EOM){
                break;
            }
        }
        if(buff[n-1] == (uint8_t)EOM){
            for (i = 0; i < NCOMMANDS; i++){
                if (buff[1] == commands[i].cmd_name){
                    commands[i].cmd_fnct(n, buff);
                    break;
                }
            }
            memset(buff, 0, sizeof buff); //Clean Array
            c = 0x01;
            n=0;
        }
        
        if(flagS1pushed){
            S1button();
            flagS1pushed=false;
            /*update_menuLCD();
            updateLCD=false;
            __delay_ms(100); //se der update do LCD muito rapido pode comecar a escrever lixo*/
        }
        
        if(PWM_on){
            if(PWM6EN==0){ //Verifica se o Timer2 e PWM esta desligado
                TMR2_StartTimer();
                PWM_Output_D4_Enable();
            }
            if(dimingLed <= 1023){ //max 1023
                dimingLed += 1;
            } else{
                dimingLed = 0;
            }
            PWM6_LoadDutyValue(dimingLed);
            __delay_ms(1);
        }

        if(modeFlag == 0){
            if(updateLCD){
                update_menuLCD();
                updateLCD=false;
            }
            /*if(PWM_on){ 
                continue;
            } else {
                SLEEP();
            }*/
        } else if(modeFlag == 1){ //All modes are active blocking
            editClock(); //Clock Edit Handler
        } else if(modeFlag == 2){
            editTemp(); //Temperature Edit Handler
        } else if(modeFlag == 3){
            editLum(); //Luminosity Edit Handler
        } else if(modeFlag == 4){
            toggleAlarms(); //Enables/Disables Alarms
        } else if(modeFlag > 4){
            modeFlag = 0;
        }
    }   
}