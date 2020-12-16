#include <stdio.h>
#include <stdlib.h>
#include <cyg/io/io.h>
#include <cyg/kernel/kapi.h>

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

//Comunicação entre thread UI e thread processing
#define CPT 0xD0  //Check period of transference
#define MPT 0xD1  //Modify period of transference
#define CTTL 0xD2 //Check threshold temperature and luminosity
#define DTTL 0xD3 //Define threshold temperature and luminosity
#define PR 0xD4   //Process registers (max, min, mean) between instants t1 and t2 (h,m,s)

#define uiID 0xCE   //ID da thread UI
#define procID 0xCF //ID da thread processing

#define NRBUF 100 //Dimensão do buffer circular do eCos
#define REGDIM 5  //Dimensão dos registos (bytes)

typedef struct mBoxMessages
{
  unsigned char data[20]; //Data to be transmited
  unsigned int cmd_dim;   //Number of bytes to be transmited
} mBoxMessage;

/*-------------------------------------------------------------------------+
| Variables
+--------------------------------------------------------------------------*/

extern Cyg_ErrNo err;
extern cyg_io_handle_t serH; //Serial Handler

extern cyg_mutex_t sharedBuffMutex; //Mutex para proteger acessos ao eCosRingBuff e os respetivos indices (iread, iwrite,...)
extern cyg_mutex_t printfMutex;     //Mutex para proteger escritas na consola, uma vez que mais que uma thread pode escrever na consola

extern cyg_handle_t TXMboxH; //TXMbos, mail box de receção de comandos na thread TX, vindos da thread UI e da thread processing.
extern cyg_handle_t uiMboxH; //uiMbox, mail box de receção de comandos na thread UI, vindos do processing thread e do RX thread.

extern cyg_sem_t newCmdSem; //Semaforo para esperar pela respostas da thread RX (Antes de enviar uma mensagem para a thread TX fazer wait)

extern cyg_handle_t procMboxH; //Mail box da processing task, em que esta irá receber comandos da thread UI e da RX.

unsigned char bufr[60]; //Buffer que contem uma copia da informação que foi enviada para a thread UI

//Variaveis do buffer circular eCos
extern unsigned char eCosRingBuff[NRBUF][REGDIM];
extern int iwrite; //Indice da proxima escrita no buffer circular
extern int iread;  //Indice da proxima leitura no buffer circular
extern int nr;     //Numero de registos válidos
extern bool flagNr;
extern int maxReadings; //Número de registos que ainda não foram lidos

mBoxMessage txMessage; //Mensagem recebida pela TX thread

bool cmd_reg_write(void)
{
  if (bufr[2] == CMD_ERROR)
  {
    return false;
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

    if (maxReadings == NRBUF)
    { //Buffer esta cheio
      iread++;
    }
    else
    {
      maxReadings++;
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
  return true;
}

void tx_th_prog(cyg_addrword_t data)
{
  cyg_mutex_lock(&printfMutex);
  printf("Working TX thread\n");
  cyg_mutex_unlock(&printfMutex);

  cyg_thread_delay(5);
  while (1)
  {
    txMessage = (*(mBoxMessage *)cyg_mbox_get(TXMboxH));
    err = cyg_io_write(serH, txMessage.data, &(txMessage.cmd_dim));
  }
}

void rx_th_prog(cyg_addrword_t data)
{
  int n = 0;
  unsigned char buf_byte[1];
  unsigned int byte = 1;

  cyg_mutex_lock(&printfMutex);
  printf("Working RX thread\n");
  cyg_mutex_unlock(&printfMutex);

  cyg_thread_delay(5);
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

        if (cyg_current_time() < t) //Ignora mensagens que demoram mais de 500ms
        {
          mBoxMessage rxMessage;
          if (bufr[1] == NMFL) //Message assincrona, para ser enviada para a processing thread
          {
            rxMessage.cmd_dim = n;
            memcpy(rxMessage.data, bufr, rxMessage.cmd_dim);
            rxMessage.data[n] = procID;

            if (!cyg_mbox_tryput(procMboxH, &rxMessage))
            {
              cyg_mutex_lock(&printfMutex);
              printf("RX write ERROR\n");
              cyg_mutex_unlock(&printfMutex);
            }
            continue;
          }

          if (bufr[1] == TRGC || bufr[1] == TRGI)
          {
            cmd_reg_write();
            memcpy(rxMessage.data, bufr, 3);
            rxMessage.data[4] = txMessage.data[txMessage.cmd_dim];
            rxMessage.cmd_dim = 4;
          }
          else
          {
            rxMessage.cmd_dim = n;
            memcpy(rxMessage.data, bufr, rxMessage.cmd_dim);
            rxMessage.data[n] = txMessage.data[txMessage.cmd_dim];
          }

          if (txMessage.data[txMessage.cmd_dim] == uiID) //Enviar para o UI thread
          {
            if (!cyg_mbox_tryput(uiMboxH, &rxMessage))
            {
              cyg_mutex_lock(&printfMutex);
              printf("RX write ERROR\n");
              cyg_mutex_unlock(&printfMutex);
            }
          }
          else //Enviar para o processing thread
          {
            if (!cyg_mbox_tryput(procMboxH, &rxMessage))
            {
              cyg_mutex_lock(&printfMutex);
              printf("RX write ERROR\n");
              cyg_mutex_unlock(&printfMutex);
            }
          }
        }
      }
    }
  }
}