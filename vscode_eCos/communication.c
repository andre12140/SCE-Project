#include <stdio.h>
#include <stdlib.h>
#include <cyg/io/io.h>
#include <cyg/kernel/kapi.h>

extern Cyg_ErrNo err;
extern cyg_io_handle_t serH;

extern cyg_mutex_t sharedBuffMutex;

extern cyg_handle_t uiMboxH;
extern cyg_handle_t TXMboxH;

extern cyg_sem_t newCmdSem;

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

#define uiID 0xCE
#define procID 0xCF

#define NRBUF 100
#define REGDIM 5

unsigned char bufr[20];

extern unsigned char eCosRingBuff[NRBUF][REGDIM];
extern int iwrite;
extern int iread;
extern int nr;
extern bool flagNr;

typedef struct mBoxMessages
{
  unsigned char data[20]; //Data to be transmited
  unsigned int cmd_dim;   //Number of bytes to be transmited
} mBoxMessage;

void cmd_reg_write(void)
{
  if (bufr[2] == CMD_ERROR)
  {
    printf("CMD_ERROR\n");
    return;
  }
  int j;
  int n = bufr[2];
  int offset;

  if (bufr[1] == TRGC)
  {
    offset = 3;
  }
  else //if (bufr[1] == TRGI)
  {
    offset = 4;
  }
  cyg_mutex_lock(&sharedBuffMutex);
  for (j = 0; j < n; j++)
  {
    memcpy(eCosRingBuff[iwrite], &bufr[offset + (j * REGDIM)], REGDIM);
    if ((nr == NRBUF) && (iread == iwrite))
    {
      iread++;
    }

    iwrite++;
    if (iwrite > NRBUF - 1)
    {
      flagNr = true;
      iwrite = 0;
    }
    if (flagNr)
    {
      nr = NRBUF;
    }
    else
    {
      nr++;
    }

    if (iread > NRBUF - 1)
    {
      iread = 0;
    }
  }
  cyg_mutex_unlock(&sharedBuffMutex);
}

mBoxMessage txMessage;
void tx_th_prog(cyg_addrword_t data)
{
  printf("Working TX thread\n");
  cyg_thread_delay(200);
  while (1)
  {
    txMessage = (*(mBoxMessage *)cyg_mbox_get(TXMboxH));
    err = cyg_io_write(serH, txMessage.data, &(txMessage.cmd_dim));
    printf("io_write err=%x, w=%d\n", err, txMessage.cmd_dim); //DEBUG
  }
}

void rx_th_prog(cyg_addrword_t data)
{
  int n = 0;
  unsigned char buf_byte[1];
  unsigned int byte = 1;

  printf("Working RX thread\n");
  cyg_thread_delay(200);
  cyg_tick_count_t t = 0;

  while (1)
  {
    err = cyg_io_read(serH, buf_byte, &byte); //Blocking by default
    if (buf_byte[0] == (unsigned char)SOM || bufr[0] == (unsigned char)SOM)
    {
      if (buf_byte[0] == (unsigned char)SOM)
      {
        n = 0;
        t = cyg_current_time() + 50;
      }
      bufr[n] = buf_byte[0];
      n++;
      if (buf_byte[0] == (unsigned char)EOM)
      {
        //printf("io_read err=%x, r=%d\n", err, n);
        if (bufr[1] == TRGC || bufr[1] == TRGI)
        {
          cmd_reg_write();
        }
        else if (cyg_current_time() < t) //Skip messages that took more than 50ms
        {
          mBoxMessage rxMessage;

          rxMessage.cmd_dim = n;
          memcpy(rxMessage.data, bufr, rxMessage.cmd_dim + 1);
          if (txMessage.data[txMessage.cmd_dim] == uiID)
          {
            cyg_mbox_tryput(uiMboxH, &rxMessage);
          }
          else //Enviar para o processing task
          {
            //cyg_mbox_tryput(procMboxH, &rxMessage);
          }
        }
        cyg_semaphore_post(&newCmdSem);
        //post sem
      }
    }
  }
}