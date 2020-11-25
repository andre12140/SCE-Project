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

/*
                         Main application
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

void cmd_rc(int, char **);

struct command_d
{
  void (*cmd_fnct)(int, char **);
  char cmd_name;
}

const commands[] = {
    {cmd_rc, 0xC0}
};

#define NCOMMANDS (sizeof(commands) / sizeof(struct command_d))

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


void cmd_rc(int, char **){

    unsigned char buff[6];
    buff[0] = (unsigned char)SOM;
    buff[1] = (unsigned char)RCLK;
    buff[2] = 10;
    buff[3] = 20;
    buff[4] = 30;
    buff[5] = (unsigned char)EOM;
    int n = 0;
    while(n<6){
        putch(buff[n]);
        n++;
    }
}

void main(void)
{    
    // initialize the device
    SYSTEM_Initialize();

    // When using interrupts, you need to set the Global and Peripheral Interrupt Enable bits
    // Use the following macros to:

    // Enable the Global Interrupts
    INTERRUPT_GlobalInterruptEnable();

    // Enable the Peripheral Interrupts
    INTERRUPT_PeripheralInterruptEnable();

    // Disable the Global Interrupts
    //INTERRUPT_GlobalInterruptDisable();

    // Disable the Peripheral Interrupts
    //INTERRUPT_PeripheralInterruptDisable();

    PORTA = 0x55;
    uint8_t c;
    char buff[10];
    int n=0;
    //printf("EcoUsart\n");
    int i=0;
    while (1)
    {
        // Add your application code
        c = getch();
        buff[n] = c;
        n++;
        if(buff[n-1] == (uint8_t)0xFE){
            buff[n-1]=(uint8_t)0xFE;
            n=0;
            
            for (i = 0; i < NCOMMANDS; i++)
                if (buff[1] == commands[i].cmd_name)
                    break;
            if (i < NCOMMANDS)
                commands[i].cmd_fnct(0, NULL);
        }
        
        
        //PORTA = c;
        //putch(c);
    }
}
/**
 End of File
*/