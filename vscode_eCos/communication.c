/***************************************************************************
| File: comando.c  -  Concretizacao de comandos (exemplo)
|
| Autor: Carlos Almeida (IST)
| Data:  Maio 2008
***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <cyg/io/io.h>
#include <cyg/kernel/kapi.h>

extern Cyg_ErrNo err;
extern cyg_io_handle_t serH;
extern cyg_sem_t TX_sem;
extern cyg_sem_t RX_sem;

#define SOM 0xFD       /* start of message */
#define EOM 0xFE       /* end of message */
#define RCLK 0xC0      /* read clock */
#define SCLK 0XC1      /* set clock */
#define RTL 0XC2       /* read temperature and luminosity */
#define RPAR 0XC3      /* read parameters */
#define MMP 0XC4       /* modify monitoring period */
#define MTA 0XC5       /* modify time alarm */
#define RALA 0XC6      /* read alarms (clock, temperature, luminosity, active/inactive) */
#define DAC 0XC7       /* define alarm clock */
#define DATL 0XC8      /* define alarm temperature and luminosity */
#define AALA 0XC9      /* activate/deactivate alarms */
#define IREG 0XCA      /* information about registers (NREG, nr, iread, iwrite)*/
#define TRGC 0XCB      /* transfer registers (curr. position)*/
#define TRGI 0XCC      /* transfer registers (index) */
#define NMFL 0XCD      /* notification memory (half) full */
#define CMD_OK 0       /* command successful */
#define CMD_ERROR 0xFF /* error in command */

extern unsigned char bufw[20];
extern unsigned int w;

extern unsigned char bufr[20];

void tx_th_prog(cyg_addrword_t data)
{
  printf("Working TX thread\n");
  cyg_thread_delay(200);
  //int i;
  while (1)
  {
    cyg_semaphore_wait(&TX_sem);
    // if (bufw[0] == (unsigned char)SOM && bufw[w - 1] == (unsigned char)EOM)
    // {
    err = cyg_io_write(serH, bufw, &w);
    /*for (i = 0; i < w; i++)
      {
        printf("%hhx\n", bufw[i]);
      }*/
    printf("io_write err=%x, w=%d\n", err, w);
    bufw[0] = 0x01;
    w = 0;
    //}
    //cyg_thread_delay(20);
  }
}

void rx_th_prog(cyg_addrword_t data)
{
  int n = 0;
  unsigned char buf_byte[1];
  unsigned int byte = 1;

  printf("Working RX thread\n");
  cyg_thread_delay(200);

  while (1)
  {
    err = cyg_io_read(serH, buf_byte, &byte); //Blocking by default
    if (buf_byte[0] == (unsigned char)SOM || bufr[0] == (unsigned char)SOM)
    {
      if (n >= 0 && buf_byte[0] == (unsigned char)SOM)
      {
        n = 0;
      }
      bufr[n] = buf_byte[0];
      n++;
      if (buf_byte[0] == (unsigned char)EOM)
      {
        //printf("io_read err=%x, r=%d\n", err, n);
        cyg_semaphore_post(&RX_sem);
        bufr[0] = 0x01;
        n = 0;
      }
    }
  }
}