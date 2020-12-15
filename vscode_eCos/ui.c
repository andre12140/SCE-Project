#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <cyg/io/io.h>
#include <cyg/kernel/kapi.h>

Cyg_ErrNo err;
cyg_io_handle_t serH;
cyg_bool_t r;

cyg_mutex_t sharedBuffMutex;

cyg_mutex_t printfMutex;

cyg_sem_t newCmdSem;

cyg_mbox uiMbox, TXMbox;

cyg_handle_t uiMboxH;
cyg_handle_t TXMboxH;
extern cyg_handle_t procMboxH;

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

#define CPT 0xD0
#define MPT 0xD1
#define CTTL 0xD2
#define DTTL 0xD3
#define PR 0xD4

#define NRBUF 100
#define REGDIM 5

unsigned char eCosRingBuff[NRBUF][REGDIM];
int iwrite = 0;
int iread = 0;
int nr = 0;
bool flagNr = false;

unsigned char bufr[100];

/*-------------------------------------------------------------------------+
| Header definition
+--------------------------------------------------------------------------*/
bool cmd_sos(int argc, char **argv);
bool cmd_sair(int argc, char **argv);
bool cmd_ini(int argc, char **argv);

void cmd_status_display(void);

bool cmd_rc(int argc, char **argv);
void cmd_rc_display(void);
bool cmd_sc(int argc, char **argv);
bool cmd_rtl(int argc, char **argv);
void cmd_rtl_display(void);
bool cmd_rp(int argc, char **argv);
void cmd_rp_display(void);
bool cmd_mmp(int argc, char **argv);
bool cmd_mta(int argc, char **argv);
bool cmd_ra(int argc, char **argv);
void cmd_ra_display(void);
bool cmd_dac(int argc, char **argv);
bool cmd_dtl(int argc, char **argv);
bool cmd_aa(int argc, char **argv);
bool cmd_ir(int argc, char **argv);
void cmd_ir_display(void);
bool cmd_trc(int argc, char **argv);
bool cmd_tri(int argc, char **argv);

bool cmd_irl(int argc, char **argv);
bool cmd_lr(int argc, char **argv);
bool cmd_dr(int argc, char **argv);

bool cmd_cpt(int argc, char **argv);
void cmd_cpt_display(void);
bool cmd_mpt(int argc, char **argv);
bool cmd_cttl(int argc, char **argv);
void cmd_cttl_display(void);
bool cmd_dttl(int argc, char **argv);
bool cmd_pr(int argc, char **argv);
void cmd_pr_display(void);

/*-------------------------------------------------------------------------+
| Variable and constants definition
+--------------------------------------------------------------------------*/
const char TitleMsg[] = "\n Application Control Monitor (tst)\n";
const char InvalMsg[] = "\nInvalid command!";

struct command_d
{
  bool (*cmd_fnct)(int, char **);
  void (*cmd_display)(void);
  char *cmd_name;
  char *cmd_help;
}

const commands[] = {
    {cmd_sos, NULL, "sos", "                 help"},

    {cmd_rc, cmd_rc_display, "rc", "                  read clock"},
    {cmd_sc, cmd_status_display, "sc", "                  set clock"},
    {cmd_rtl, cmd_rtl_display, "rtl", "                 read temperature and luminosity"},
    {cmd_rp, cmd_rp_display, "rp", "                  read parameters (PMON, TALA)"},
    {cmd_mmp, cmd_status_display, "mmp", "<p>              modify monitoring period (seconds - 0 deactivate)"},
    {cmd_mta, cmd_status_display, "mta", "<t>              modify time alarm (seconds)"},
    {cmd_ra, cmd_ra_display, "ra", "                  read alarms (clock, temperature, luminosity, active/inactive-1/0)"},
    {cmd_dac, cmd_status_display, "dac", "<h> <m> <s>      define alarm clock"},
    {cmd_dtl, cmd_status_display, "dtl", "<t> <l>          define alarm temperature and luminosity"},
    {cmd_aa, cmd_status_display, "aa", "<a>               activate/deactivate alarms (1/0)"},
    {cmd_ir, cmd_ir_display, "ir", "<p>              information about registers (NREG, nr, iread, iwrite)"},
    {cmd_trc, cmd_status_display, "trc", "<n>              transfer n registers from current iread position"},
    {cmd_tri, cmd_status_display, "tri", "<n> <i>          transfer n registers from index i (0 - oldest)"},

    {cmd_irl, NULL, "irl", "                 information about local registers (NRBUF, nr, iread, iwrite)"},
    {cmd_lr, NULL, "lr", "<n> <i>          list n registers (local memory) from index i (0 - oldest)"},
    {cmd_dr, NULL, "dr", "                 delete registers (local memory)"},

    {cmd_cpt, cmd_cpt_display, "cpt", "                 check period of transference"},
    {cmd_mpt, cmd_status_display, "mpt", "<p>              modify period of transference (minutes - 0 deactivate)"},
    {cmd_cttl, cmd_cttl_display, "cttl", "                 check threshold temperature and luminosity for processing"},
    {cmd_dttl, cmd_status_display, "dttl", "<t> <l>          dene threshold temperature and luminosity for processing"},
    {cmd_pr, cmd_pr_display, "pr", "[h1 m1 s1 [h2 m2 s2]]  process registers (max, min, mean) between instants t1 and t2 (h,m,s)"},

    {cmd_sair, NULL, "sair", "                sair"},
    {cmd_ini, NULL, "ini", "<d>              inicializar dispositivo (0/1) ser0/ser1"}};

#define NCOMMANDS (sizeof(commands) / sizeof(struct command_d))
#define ARGVECSIZE 10
#define MAX_LINE 50

typedef struct mBoxMessages
{
  unsigned char data[20]; //Data to be transmited
  unsigned int cmd_dim;   //Number of bytes to be transmited
} mBoxMessage;

mBoxMessage m_w; //Mensagem de escrita para o Proc

///////////////////////////////////////////
// SOS TA MARADO

/*-------------------------------------------------------------------------+
| Function: cmd_sos - provides a rudimentary help
+--------------------------------------------------------------------------*/
bool cmd_sos(int argc, char **argv)
{
  cyg_mutex_lock(&printfMutex);
  int i;
  printf("%s\n", TitleMsg);
  for (i = 0; i < NCOMMANDS; i++)
    printf("%s %s\n", commands[i].cmd_name, commands[i].cmd_help);
  cyg_mutex_unlock(&printfMutex);
  return true;
}

/*-------------------------------------------------------------------------+
| Function: cmd_sair - termina a aplicacao
+--------------------------------------------------------------------------*/
bool cmd_sair(int argc, char **argv)
{
  //Falta terminar outras threads
  exit(0);
  return true;
}

/*-------------------------------------------------------------------------+
| Function: cmd_ini - inicializar dispositivo
+--------------------------------------------------------------------------*/
bool cmd_ini(int argc, char **argv)
{
  cyg_mutex_lock(&printfMutex);
  printf("io_lookup\n");
  if ((argc > 1) && (argv[1][0] = '1'))
    err = cyg_io_lookup("/dev/ser1", &serH);
  else
    err = cyg_io_lookup("/dev/ser0", &serH);
  printf("lookup err=%x\n", err);
  cyg_mutex_unlock(&printfMutex);
  return true;
}

/*-------------------------------------------------------------------------+
| Function: cmd_status_display - imprime resposta de funcoes de escrita
+--------------------------------------------------------------------------*/
void cmd_status_display()
{
  if (bufr[2] == CMD_OK)
  {
    printf("CMD_OK\n");
  }
  else if (bufr[2] == CMD_ERROR)
  {
    printf("CMD_ERROR\n");
  }
}

/*-------------------------------------------------------------------------+
| Function: cmd_rc - read clock
+--------------------------------------------------------------------------*/
bool cmd_rc(int argc, char **argv)
{
  if (argc != 1)
  {
    cyg_mutex_lock(&printfMutex);
    printf("Bad Inputs\n");
    cyg_mutex_unlock(&printfMutex);
    return false;
  }
  mBoxMessage m;
  unsigned char bufw[4];
  m.cmd_dim = 3;
  bufw[3] = (unsigned char)uiID;
  bufw[2] = (unsigned char)EOM;
  bufw[1] = (unsigned char)RCLK;
  bufw[0] = (unsigned char)SOM;
  memcpy(m.data, bufw, m.cmd_dim + 1);

  cyg_mbox_tryput(TXMboxH, &m);
  return true;
}

void cmd_rc_display()
{
  if (bufr[2] == CMD_ERROR)
  {
    printf("CMD_ERROR\n");
    return;
  }
  printf("Horas = %d\n", bufr[2]);
  printf("Minutos = %d\n", bufr[3]);
  printf("Segundos = %d\n", bufr[4]);
}

/*-------------------------------------------------------------------------+
| Function: cmd_sc - set clock
+--------------------------------------------------------------------------*/
bool cmd_sc(int argc, char **argv)
{
  if (argc != 4)
  {
    cyg_mutex_lock(&printfMutex);
    printf("Bad inputs\n");
    cyg_mutex_unlock(&printfMutex);
    return false;
  }
  unsigned int h, min, s;
  sscanf(argv[1], "%d", &h);
  sscanf(argv[2], "%d", &min);
  sscanf(argv[3], "%d", &s);

  //printf("Horas = %d\n", h);
  //printf("Minutos = %d\n", m);
  //printf("Segundos = %d\n", s);

  mBoxMessage m;
  m.cmd_dim = 6;
  unsigned char bufw[m.cmd_dim + 1];
  bufw[6] = (unsigned char)uiID;
  bufw[5] = (unsigned char)EOM;
  bufw[4] = s;
  bufw[3] = min;
  bufw[2] = h;
  bufw[1] = (unsigned char)SCLK;
  bufw[0] = (unsigned char)SOM;
  memcpy(m.data, bufw, m.cmd_dim + 1);
  cyg_mbox_tryput(TXMboxH, &m);
  return true;
}

/*-------------------------------------------------------------------------+
| Function: cmd_rtl - read Temperature and Luminosity
+--------------------------------------------------------------------------*/
bool cmd_rtl(int argc, char **argv)
{
  if (argc != 1)
  {
    cyg_mutex_lock(&printfMutex);
    printf("Bad inputs\n");
    cyg_mutex_unlock(&printfMutex);
    return false;
  }
  mBoxMessage m;
  m.cmd_dim = 3;
  unsigned char bufw[m.cmd_dim + 1];
  bufw[3] = (unsigned char)uiID;
  bufw[2] = (unsigned char)EOM;
  bufw[1] = (unsigned char)RTL;
  bufw[0] = (unsigned char)SOM;
  memcpy(m.data, bufw, m.cmd_dim + 1);
  cyg_mbox_tryput(TXMboxH, &m);
  return true;
}

void cmd_rtl_display()
{
  if (bufr[2] == CMD_ERROR)
  {
    printf("CMD_ERROR\n");
    return;
  }
  printf("Temp = %d\n", bufr[2]);
  printf("Lum = %d\n", bufr[3]);
}

/*-------------------------------------------------------------------------+
| Function: cmd_rp - read parameters (PMON, TALA)
+--------------------------------------------------------------------------*/
bool cmd_rp(int argc, char **argv)
{
  if (argc != 1)
  {
    cyg_mutex_lock(&printfMutex);
    printf("Bad inputs\n");
    cyg_mutex_unlock(&printfMutex);
    return false;
  }
  mBoxMessage m;
  m.cmd_dim = 3;
  unsigned char bufw[m.cmd_dim + 1];
  bufw[3] = (unsigned char)uiID;
  bufw[2] = (unsigned char)EOM;
  bufw[1] = (unsigned char)RPAR;
  bufw[0] = (unsigned char)SOM;
  memcpy(m.data, bufw, m.cmd_dim + 1);
  cyg_mbox_tryput(TXMboxH, &m);
  return true;
}

void cmd_rp_display()
{
  printf("PMON = %d\n", bufr[2]);
  printf("TALA = %d\n", bufr[3]);
}

/*-------------------------------------------------------------------------+
| Function: cmd_mmp - modify monitoring period
+--------------------------------------------------------------------------*/
bool cmd_mmp(int argc, char **argv)
{
  if (argc != 2)
  {
    cyg_mutex_lock(&printfMutex);
    printf("Bad inputs\n");
    cyg_mutex_unlock(&printfMutex);
    return false;
  }
  unsigned int p;
  sscanf(argv[1], "%d", &p);

  mBoxMessage m;
  m.cmd_dim = 4;
  unsigned char bufw[m.cmd_dim + 1];
  bufw[4] = (unsigned char)uiID;
  bufw[3] = (unsigned char)EOM;
  bufw[2] = p;
  bufw[1] = (unsigned char)MMP;
  bufw[0] = (unsigned char)SOM;
  memcpy(m.data, bufw, m.cmd_dim + 1);
  cyg_mbox_tryput(TXMboxH, &m);
  return true;
}

/*-------------------------------------------------------------------------+
| Function: cmd_mta - modify time alarm (seconds)
+--------------------------------------------------------------------------*/
bool cmd_mta(int argc, char **argv)
{
  if (argc != 2)
  {
    cyg_mutex_lock(&printfMutex);
    printf("Bad inputs\n");
    cyg_mutex_unlock(&printfMutex);
    return false;
  }
  unsigned int s;
  sscanf(argv[1], "%d", &s);

  mBoxMessage m;
  m.cmd_dim = 4;
  unsigned char bufw[m.cmd_dim + 1];
  bufw[4] = (unsigned char)uiID;
  bufw[3] = (unsigned char)EOM;
  bufw[2] = s;
  bufw[1] = (unsigned char)MTA;
  bufw[0] = (unsigned char)SOM;
  memcpy(m.data, bufw, m.cmd_dim + 1);
  cyg_mbox_tryput(TXMboxH, &m);
  return true;
}

/*-------------------------------------------------------------------------+
| Function: cmd_ra - read Alarm values
+--------------------------------------------------------------------------*/
bool cmd_ra(int argc, char **argv)
{
  if (argc != 1)
  {
    cyg_mutex_lock(&printfMutex);
    printf("Bad inputs\n");
    cyg_mutex_unlock(&printfMutex);
    return false;
  }
  mBoxMessage m;
  m.cmd_dim = 3;
  unsigned char bufw[m.cmd_dim + 1];
  bufw[3] = (unsigned char)uiID;
  bufw[2] = (unsigned char)EOM;
  bufw[1] = (unsigned char)RALA;
  bufw[0] = (unsigned char)SOM;
  memcpy(m.data, bufw, m.cmd_dim + 1);
  cyg_mbox_tryput(TXMboxH, &m);

  return true;
}

void cmd_ra_display()
{
  if (bufr[2] == CMD_ERROR)
  {
    printf("CMD_ERROR\n");
    return;
  }
  printf("Clock Alarm = %02d : %02d : %02d\n", bufr[2], bufr[3], bufr[4]);
  printf("Temp Alarm = %d\n", bufr[5]);
  printf("Lum Alarm = %d\n", bufr[6]);
  printf("Alarm State = %d\n", bufr[7]);
}

/*-------------------------------------------------------------------------+
| Function: cmd_dac - define alarm clock
+--------------------------------------------------------------------------*/
bool cmd_dac(int argc, char **argv)
{
  if (argc != 4)
  {
    cyg_mutex_lock(&printfMutex);
    printf("Bad inputs\n");
    cyg_mutex_unlock(&printfMutex);
    return false;
  }
  unsigned int h, min, s;
  sscanf(argv[1], "%d", &h);
  sscanf(argv[2], "%d", &min);
  sscanf(argv[3], "%d", &s);

  //printf("Horas = %d\n", h);
  //printf("Minutos = %d\n", m);
  //printf("Segundos = %d\n", s);

  mBoxMessage m;
  m.cmd_dim = 6;
  unsigned char bufw[m.cmd_dim + 1];
  bufw[6] = (unsigned char)uiID;
  bufw[5] = (unsigned char)EOM;
  bufw[4] = s;
  bufw[3] = min;
  bufw[2] = h;
  bufw[1] = (unsigned char)DAC;
  bufw[0] = (unsigned char)SOM;
  memcpy(m.data, bufw, m.cmd_dim + 1);
  cyg_mbox_tryput(TXMboxH, &m);

  return true;
}

/*-------------------------------------------------------------------------+
| Function: cmd_dtl - define Temperature and Luminosity Alarm
+--------------------------------------------------------------------------*/
bool cmd_dtl(int argc, char **argv)
{
  if (argc != 3)
  {
    cyg_mutex_lock(&printfMutex);
    printf("Bad inputs\n");
    cyg_mutex_unlock(&printfMutex);
    return false;
  }
  unsigned int T, L;
  sscanf(argv[1], "%d", &T);
  sscanf(argv[2], "%d", &L);

  mBoxMessage m;
  m.cmd_dim = 5;
  unsigned char bufw[m.cmd_dim + 1];
  bufw[5] = (unsigned char)uiID;
  bufw[4] = (unsigned char)EOM;
  bufw[3] = L;
  bufw[2] = T;
  bufw[1] = (unsigned char)DATL;
  bufw[0] = (unsigned char)SOM;
  memcpy(m.data, bufw, m.cmd_dim + 1);
  cyg_mbox_tryput(TXMboxH, &m);
  return true;
}

/*-------------------------------------------------------------------------+
| Function: cmd_aa - activate/deactivate alarms (1/0)
+--------------------------------------------------------------------------*/
bool cmd_aa(int argc, char **argv)
{
  if (argc != 2)
  {
    cyg_mutex_lock(&printfMutex);
    printf("Bad inputs\n");
    cyg_mutex_unlock(&printfMutex);
    return false;
  }
  unsigned int a;
  sscanf(argv[1], "%d", &a);

  mBoxMessage m;
  m.cmd_dim = 4;
  unsigned char bufw[m.cmd_dim + 1];
  bufw[4] = (unsigned char)uiID;
  bufw[3] = (unsigned char)EOM;
  bufw[2] = a;
  bufw[1] = (unsigned char)AALA;
  bufw[0] = (unsigned char)SOM;
  memcpy(m.data, bufw, m.cmd_dim + 1);
  cyg_mbox_tryput(TXMboxH, &m);
  return true;
}

/*-------------------------------------------------------------------------+
| Function: cmd_ir - information about registers (NREG, nr, iread, iwrite)
+--------------------------------------------------------------------------*/
bool cmd_ir(int argc, char **argv)
{
  if (argc != 1)
  {
    cyg_mutex_lock(&printfMutex);
    printf("Bad inputs\n");
    cyg_mutex_unlock(&printfMutex);
    return false;
  }

  mBoxMessage m;
  m.cmd_dim = 3;
  unsigned char bufw[m.cmd_dim + 1];
  bufw[3] = (unsigned char)uiID;
  bufw[2] = (unsigned char)EOM;
  bufw[1] = (unsigned char)IREG;
  bufw[0] = (unsigned char)SOM;
  memcpy(m.data, bufw, m.cmd_dim + 1);
  cyg_mbox_tryput(TXMboxH, &m);

  return true;
}

void cmd_ir_display()
{
  if (bufr[2] == CMD_ERROR)
  {
    printf("CMD_ERROR\n");
    return;
  }
  printf("NREG = %d\n", bufr[2]);
  printf("NR = %d\n", bufr[3]);
  printf("iRead = %d\n", bufr[4]);
  printf("iWrite = %d\n", bufr[5]);
}

/*-------------------------------------------------------------------------+
| Function: cmd_trc - transfer n registers from current iread position
+--------------------------------------------------------------------------*/
bool cmd_trc(int argc, char **argv)
{
  if (argc != 2)
  {
    cyg_mutex_lock(&printfMutex);
    printf("Bad inputs\n");
    cyg_mutex_unlock(&printfMutex);
    return false;
  }
  unsigned int n;
  sscanf(argv[1], "%d", &n);

  mBoxMessage m;
  m.cmd_dim = 4;
  unsigned char bufw[m.cmd_dim + 1];
  bufw[4] = (unsigned char)uiID;
  bufw[3] = (unsigned char)EOM;
  bufw[2] = n;
  bufw[1] = (unsigned char)TRGC;
  bufw[0] = (unsigned char)SOM;
  memcpy(m.data, bufw, m.cmd_dim + 1);
  cyg_mbox_tryput(TXMboxH, &m);
  return true;
}

/*-------------------------------------------------------------------------+
| Function: cmd_tri - transfer n registers from index i (0 - oldest)
+--------------------------------------------------------------------------*/
bool cmd_tri(int argc, char **argv)
{
  if (argc != 3)
  {
    cyg_mutex_lock(&printfMutex);
    printf("Bad inputs\n");
    cyg_mutex_unlock(&printfMutex);
    return false;
  }
  unsigned int n, i;
  sscanf(argv[1], "%d", &n);
  sscanf(argv[2], "%d", &i);

  mBoxMessage m;
  m.cmd_dim = 5;
  unsigned char bufw[m.cmd_dim + 1];
  bufw[5] = (unsigned char)uiID;
  bufw[4] = (unsigned char)EOM;
  bufw[3] = i;
  bufw[2] = n;
  bufw[1] = (unsigned char)TRGI;
  bufw[0] = (unsigned char)SOM;
  memcpy(m.data, bufw, m.cmd_dim + 1);
  cyg_mbox_tryput(TXMboxH, &m);
  return true;
}

/*-------------------------------------------------------------------------+
| Function: cmd_irl - information about local registers (NRBUF, nr, iread, iwrite)
+--------------------------------------------------------------------------*/

bool cmd_irl(int argc, char **argv)
{
  cyg_mutex_lock(&printfMutex);
  printf("NRBUF = %d\n", NRBUF);
  printf("NR = %d\n", nr);
  printf("iRead = %d\n", iread);
  printf("iWrite = %d\n", iwrite);
  cyg_mutex_unlock(&printfMutex);
  return true;
}

/*-------------------------------------------------------------------------+
| Function: cmd_lr - list n registers (local memory) from index i (0 - oldest)
+--------------------------------------------------------------------------*/

bool cmd_lr(int argc, char **argv)
{
  if (argc < 3 && argc > 4)
  {
    cyg_mutex_lock(&printfMutex);
    printf("Bad Inputs\n");
    cyg_mutex_unlock(&printfMutex);
    return false;
  }
  int n = atoi(argv[1]);
  int i;
  cyg_mutex_lock(&sharedBuffMutex);
  if (argc == 3)
  {
    int index = atoi(argv[2]);

    int startingIndex = iwrite + index;
    if (startingIndex >= NRBUF)
    {
      startingIndex = index - (NRBUF - iwrite);
    }
    if (nr != NRBUF)
    {
      startingIndex = index;
    }
    int maxReadings = iwrite - startingIndex;
    if (maxReadings < 0)
    {
      maxReadings = iwrite + (NRBUF - startingIndex);
    }

    if ((n > nr) || (maxReadings < n))
    {
      cyg_mutex_lock(&printfMutex);
      printf("ERROR");
      cyg_mutex_unlock(&printfMutex);
      return false;
    }
    i = startingIndex;
    while (n)
    {
      cyg_mutex_lock(&printfMutex);
      printf("Clock = %02d : %02d : %02d\n", eCosRingBuff[i][0], eCosRingBuff[i][1], eCosRingBuff[i][2]);
      printf("Temp = %d\n", eCosRingBuff[i][3]);
      printf("Lum = %d\n\n", eCosRingBuff[i][4]);
      cyg_mutex_unlock(&printfMutex);
      if (iread == i)
      {
        iread++;
      }
      i++;
      n--;
      if (i >= NRBUF)
      {
        i = 0;
      }
    }
  }
  else //index is omitted
  {
    int maxReadings = (iwrite - iread);
    if (maxReadings < 0)
    {
      maxReadings = iwrite + (NRBUF - iread);
    }
    if ((n > nr) || (n > maxReadings))
    {
      cyg_mutex_lock(&printfMutex);
      printf("ERROR");
      cyg_mutex_unlock(&printfMutex);
      return false;
    }
    for (i = 0; i < n; i++)
    { //Read n registers
      cyg_mutex_lock(&printfMutex);
      printf("Clock = %02d : %02d : %02d\n", eCosRingBuff[iread][0], eCosRingBuff[iread][1], eCosRingBuff[iread][2]);
      printf("Temp = %d\n", eCosRingBuff[iread][3]);
      printf("Lum = %d\n\n", eCosRingBuff[iread][4]);
      cyg_mutex_unlock(&printfMutex);
      iread++;
      if (iread > NRBUF - 1)
      {
        iread = 0;
      }
    }
  }
  cyg_mutex_unlock(&sharedBuffMutex);
  return true;
}

/*-------------------------------------------------------------------------+
| Function: cmd_dr - delete registers (local memory)
+--------------------------------------------------------------------------*/

bool cmd_dr(int argc, char **argv)
{
  cyg_mutex_lock(&sharedBuffMutex);
  memset(eCosRingBuff, 0, NRBUF * REGDIM); //Limpar buffer
  iwrite = 0;
  iread = 0;
  nr = 0;
  flagNr = false;
  cyg_mutex_unlock(&sharedBuffMutex);
  cyg_mutex_lock(&printfMutex);
  printf("CMD_OK");
  cyg_mutex_unlock(&printfMutex);
  return true;
}

/*-------------------------------------------------------------------------+
| Function: cmd_cpt - check period of transference
+--------------------------------------------------------------------------*/

bool cmd_cpt(int argc, char **argv)
{
  if (argc != 1)
  {
    cyg_mutex_lock(&printfMutex);
    printf("Bad inputs\n");
    cyg_mutex_unlock(&printfMutex);
    return false;
  }

  m_w.cmd_dim = 3;
  m_w.data[3] = (unsigned char)uiID;
  m_w.data[2] = (unsigned char)EOM;
  m_w.data[1] = (unsigned char)CPT;
  m_w.data[0] = (unsigned char)SOM;

  if (!cyg_mbox_tryput(procMboxH, &m_w))
  {
    cyg_mutex_lock(&printfMutex);
    printf("Proc Mail Box full (UI_cpt)");
    cyg_mutex_unlock(&printfMutex);
    return false;
  }
  return true;
}

void cmd_cpt_display()
{
  printf("Period of transference = %d\n", bufr[2]);
}

/*-------------------------------------------------------------------------+
| Function: cmd_mpt - modify period of transference (minutes - 0 deactivate)
+--------------------------------------------------------------------------*/

bool cmd_mpt(int argc, char **argv)
{
  if (argc != 2)
  {
    cyg_mutex_lock(&printfMutex);
    printf("Bad inputs\n");
    cyg_mutex_unlock(&printfMutex);
    return false;
  }

  unsigned int p;
  sscanf(argv[1], "%d", &p);

  m_w.cmd_dim = 4;
  m_w.data[4] = (unsigned char)uiID;
  m_w.data[3] = (unsigned char)EOM;
  m_w.data[1] = (unsigned char)p;
  m_w.data[1] = (unsigned char)MPT;
  m_w.data[0] = (unsigned char)SOM;

  cyg_mbox_tryput(procMboxH, &m_w);
  return true;
}

/*-------------------------------------------------------------------------+
| Function: cmd_cttl - check threshold temperature and luminosity for processing
+--------------------------------------------------------------------------*/

bool cmd_cttl(int argc, char **argv)
{
  if (argc != 1)
  {
    cyg_mutex_lock(&printfMutex);
    printf("Bad inputs\n");
    cyg_mutex_unlock(&printfMutex);
    return false;
  }
  m_w.cmd_dim = 3;
  m_w.data[3] = (unsigned char)uiID;
  m_w.data[2] = (unsigned char)EOM;
  m_w.data[1] = (unsigned char)CTTL;
  m_w.data[0] = (unsigned char)SOM;

  cyg_mbox_tryput(procMboxH, &m_w);
  return true;
}

void cmd_cttl_display()
{
  printf("eCos Temp threshold = %d\n", bufr[2]);
  printf("eCos Lum threshold = %d\n", bufr[3]);
}

/*-------------------------------------------------------------------------+
| Function: cmd_dttl - define threshold temperature and luminosity for processing
+--------------------------------------------------------------------------*/

bool cmd_dttl(int argc, char **argv)
{
  if (argc != 3)
  {
    cyg_mutex_lock(&printfMutex);
    printf("Bad inputs\n");
    cyg_mutex_unlock(&printfMutex);
    return false;
  }

  unsigned int t, l;
  sscanf(argv[1], "%d", &t);
  sscanf(argv[2], "%d", &l);

  m_w.cmd_dim = 5;
  m_w.data[5] = (unsigned char)uiID;
  m_w.data[4] = (unsigned char)EOM;
  m_w.data[3] = (unsigned char)l;
  m_w.data[2] = (unsigned char)t;
  m_w.data[1] = (unsigned char)DTTL;
  m_w.data[0] = (unsigned char)SOM;

  cyg_mbox_tryput(procMboxH, &m_w);
  return true;
}

/*-------------------------------------------------------------------------+
| Function: cmd_pr - process registers (max, min, mean) between instants t1 and t2 (h,m,s)
+--------------------------------------------------------------------------*/

bool cmd_pr(int argc, char **argv)
{
  if (!(argc == 1 || argc == 4 || argc == 7))
  {
    cyg_mutex_lock(&printfMutex);
    printf("Bad inputs\n");
    cyg_mutex_unlock(&printfMutex);
    return false;
  }

  if (argc == 1)
  {
    m_w.data[3] = (unsigned char)uiID;
    m_w.data[2] = (unsigned char)EOM;
    m_w.cmd_dim = 3;
  }
  else
  {
    unsigned int h1, m1, s1;
    sscanf(argv[1], "%d", &h1);
    sscanf(argv[2], "%d", &m1);
    sscanf(argv[3], "%d", &s1);
    m_w.data[6] = (unsigned char)uiID;
    m_w.data[5] = (unsigned char)EOM;
    m_w.data[4] = (unsigned char)s1;
    m_w.data[3] = (unsigned char)m1;
    m_w.data[2] = (unsigned char)h1;
    m_w.cmd_dim = 6;

    if (argc == 7)
    {
      unsigned int h2, m2, s2;
      sscanf(argv[4], "%d", &h2);
      sscanf(argv[5], "%d", &m2);
      sscanf(argv[6], "%d", &s2);
      m_w.data[9] = (unsigned char)uiID;
      m_w.data[8] = (unsigned char)EOM;
      m_w.data[7] = (unsigned char)s2;
      m_w.data[6] = (unsigned char)m2;
      m_w.data[5] = (unsigned char)h2;
      m_w.cmd_dim = 9;
    }
  }

  m_w.data[1] = (unsigned char)PR;
  m_w.data[0] = (unsigned char)SOM;

  cyg_mbox_tryput(procMboxH, &m_w);
  return true;
}

void cmd_pr_display()
{
  if (bufr[2] == CMD_ERROR)
  {
    printf("CMD_ERROR\n");
    return;
  }
  printf("Temperature: Max = %d, Min = %d, Mean = %d\n", bufr[2], bufr[3], bufr[4]);
  printf("Luminosity: Max = %d, Min = %d, Mean = %d\n", bufr[5], bufr[6], bufr[7]);
}

/*-------------------------------------------------------------------------+
| Function: getline        (called from monitor) 
+--------------------------------------------------------------------------*/
int my_getline(char **argv, int argvsize)
{
  static char line[MAX_LINE];
  char *p;
  int argc;

  fgets(line, MAX_LINE, stdin);

  /* Break command line into an o.s. like argument vector,
     i.e. compliant with the (int argc, char **argv) specification --------*/

  for (argc = 0, p = line; (*line != '\0') && (argc < argvsize); p = NULL, argc++)
  {
    p = strtok(p, " \t\n");
    argv[argc] = p;
    if (p == NULL)
      return argc;
  }
  argv[argc] = p;
  return argc;
}

/*-------------------------------------------------------------------------+
| Function: monitor        (called from ui_th_prog) 
+--------------------------------------------------------------------------*/
mBoxMessage *m;
void monitor(void)
{
  static char *argv[ARGVECSIZE + 1], *p;
  int argc, i;
  cyg_mutex_lock(&printfMutex);
  printf("%s Type sos for help\n", TitleMsg);
  cyg_mutex_unlock(&printfMutex);
  for (;;)
  {
    cyg_mutex_lock(&printfMutex);
    printf("\nCmd> ");
    cyg_mutex_unlock(&printfMutex);
    /* Reading and parsing command line  ----------------------------------*/
    if ((argc = my_getline(argv, ARGVECSIZE)) > 0)
    {
      for (p = argv[0]; *p != '\0'; *p = tolower(*p), p++)
        ;
      for (i = 0; i < NCOMMANDS; i++)
        if (strcmp(argv[0], commands[i].cmd_name) == 0)
          break;
      /* Executing commands -----------------------------------------------*/
      if (i < NCOMMANDS)
      {
        if (((i > 0) && (i < 14))) //Commands to send to PIC
        {
          if (!cyg_semaphore_timed_wait(&newCmdSem, cyg_current_time() + 50)) //So ocorre quando for para fazer uma escrita do TX
          {
            continue; //Acontece quando o comando anterior ainda nao foi totalmete processado (RX ainda nao deu post)
          }
        }
        bool cmd_exec_ret = commands[i].cmd_fnct(argc, argv);

        if (!cmd_exec_ret) //If arguments were invalid
        {
          continue;
        }
        if (((i > 0) && (i < 14)) || ((i > 16) && (i < 22))) //Receive commands from proc and RX
        {
          m = (mBoxMessage *)cyg_mbox_timed_get(uiMboxH, cyg_current_time() + 50); //500ms
          if (((i > 0) && (i < 14)))
          {
            cyg_semaphore_post(&newCmdSem);
          }
          //post sem

          if (m != NULL)
          {
            memcpy(bufr, (*m).data, (*m).cmd_dim);
            cyg_mutex_lock(&printfMutex);
            commands[i].cmd_display();
            cyg_mutex_unlock(&printfMutex);
            //printf("Fazer prints (UI)\n");
          }
          else
          {
            cyg_mutex_lock(&printfMutex);
            printf("Time to recieve message exceeded (UI)");
            cyg_mutex_unlock(&printfMutex);
          }
        }
      }
      else
      {
        cyg_mutex_lock(&printfMutex);
        printf("%s", InvalMsg);
        cyg_mutex_unlock(&printfMutex);
      }
    } /* if my_getline */
  }   /* forever */
}

void ui_th_prog(cyg_addrword_t data)
{
  cyg_mbox_create(&uiMboxH, &uiMbox);
  cyg_mbox_create(&TXMboxH, &TXMbox);

  cyg_semaphore_init(&newCmdSem, 1);

  cyg_mutex_init(&sharedBuffMutex);
  cyg_mutex_init(&printfMutex);

  cyg_mutex_lock(&printfMutex);
  printf("Working UI thread\n");
  cyg_mutex_unlock(&printfMutex);

  monitor();
}
