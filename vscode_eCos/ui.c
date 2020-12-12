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

cyg_sem_t newCmdSem;

cyg_mbox uiMbox, TXMbox;

cyg_handle_t uiMboxH;
cyg_handle_t TXMboxH;

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
void cmd_reg_display(void); //DEBUG
//void cmd_reg_write(void);

bool cmd_irl(int argc, char **argv);
bool cmd_lr(int argc, char **argv);
bool cmd_dr(int argc, char **argv);

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
    {cmd_trc, NULL, "trc", "<n>              transfer n registers from current iread position"},
    {cmd_tri, NULL, "tri", "<n> <i>          transfer n registers from index i (0 - oldest)"},

    {cmd_irl, NULL, "irl", "                 information about local registers (NRBUF, nr, iread, iwrite)"},
    {cmd_lr, NULL, "lr", "<n> <i>          list n registers (local memory) from index i (0 - oldest)"},
    {cmd_dr, NULL, "dr", "                 delete registers (local memory)"},

    // {cmd_cpt, "cpt", "                 check period of transference"},
    // {cmd_mpt, "mpt", "<p>              modify period of transference (minutes - 0 deactivate)"},
    // {cmd_cttl, "cttl", "                 check threshold temperature and luminosity for processing"},
    // {cmd_dttl, "dttl", "<t> <l>          dene threshold temperature and luminosity for processing"},
    // {cmd_pr, "pr", "[h1 m1 s1 [h2 m2 s2]]  process registers (max, min, mean) between instants t1 and t2 (h,m,s)"},

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

///////////////////////////////////////////
// SOS TA MARADO

/*-------------------------------------------------------------------------+
| Function: cmd_sos - provides a rudimentary help
+--------------------------------------------------------------------------*/
bool cmd_sos(int argc, char **argv)
{
  int i;

  printf("%s\n", TitleMsg);
  for (i = 0; i < NCOMMANDS; i++)
    printf("%s %s\n", commands[i].cmd_name, commands[i].cmd_help);
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
  printf("io_lookup\n");
  if ((argc > 1) && (argv[1][0] = '1'))
    err = cyg_io_lookup("/dev/ser1", &serH);
  else
    err = cyg_io_lookup("/dev/ser0", &serH);
  printf("lookup err=%x\n", err);
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
    printf("Bad inputs\n");
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
    printf("Bad inputs\n");
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
    printf("Bad inputs\n");
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
    printf("Bad inputs\n");
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
    printf("Bad inputs\n");
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
    printf("Bad inputs\n");
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
    printf("Bad inputs\n");
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
    printf("Bad inputs\n");
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
    printf("Bad inputs\n");
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
    printf("Bad inputs\n");
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
    printf("Bad inputs\n");
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
    printf("Bad inputs\n");
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

//So para debug
void cmd_reg_display()
{
  if (bufr[2] == CMD_ERROR)
  {
    printf("CMD_ERROR\n");
    return;
  }
  int i;
  int n = bufr[2];

  if (bufr[1] == TRGC)
  {
    printf("TRC\n");
    for (i = 3; i < (n * 5) + 3; i += 5)
    {
      printf("Clock = %02d : %02d : %02d\n", bufr[i], bufr[i + 1], bufr[i + 2]);
      printf("Temp = %d\n", bufr[i + 3]);
      printf("Lum = %d\n\n", bufr[i + 4]);
    }
  }

  if (bufr[1] == TRGI)
  {
    printf("TRI\n");
    for (i = 4; i < (n * 5) + 4; i += 5)
    {
      printf("Clock = %02d : %02d : %02d\n", bufr[i], bufr[i + 1], bufr[i + 2]);
      printf("Temp = %d\n", bufr[i + 3]);
      printf("Lum = %d\n\n", bufr[i + 4]);
    }
  }
}

/*-------------------------------------------------------------------------+
| Function: cmd_irl - information about local registers (NRBUF, nr, iread, iwrite)
+--------------------------------------------------------------------------*/

bool cmd_irl(int argc, char **argv)
{
  printf("NRBUF = %d\n", NRBUF);
  printf("NR = %d\n", nr);
  printf("iRead = %d\n", iread);
  printf("iWrite = %d\n", iwrite);
  return true;
}

/*-------------------------------------------------------------------------+
| Function: cmd_lr - list n registers (local memory) from index i (0 - oldest)
+--------------------------------------------------------------------------*/

bool cmd_lr(int argc, char **argv)
{
  if (argc < 3 && argc > 4)
  {
    printf("Bad Inputs\n");
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
    if (maxReadings <= 0)
    {
      maxReadings = iwrite + (NRBUF - startingIndex);
    }

    if ((n > nr) || (maxReadings < n))
    {
      printf("ERROR");
      return false;
    }
    i = startingIndex;
    while (n)
    {
      printf("Clock = %02d : %02d : %02d\n", eCosRingBuff[i][0], eCosRingBuff[i][1], eCosRingBuff[i][2]);
      printf("Temp = %d\n", eCosRingBuff[i][3]);
      printf("Lum = %d\n\n", eCosRingBuff[i][4]);
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
    if (maxReadings <= 0)
    {
      maxReadings = iwrite + (NRBUF - iread);
    }
    if ((n > nr) || (n > maxReadings))
    {
      printf("ERROR");
      return false;
    }
    for (i = 0; i < n; i++)
    { //Read n registers
      printf("Clock = %02d : %02d : %02d\n", eCosRingBuff[iread][0], eCosRingBuff[iread][1], eCosRingBuff[iread][2]);
      printf("Temp = %d\n", eCosRingBuff[iread][3]);
      printf("Lum = %d\n\n", eCosRingBuff[iread][4]);
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
  printf("CMD_OK");
  return true;
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

  printf("%s Type sos for help\n", TitleMsg);
  for (;;)
  {
    printf("\nCmd> ");
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
        if (!cyg_semaphore_timed_wait(&newCmdSem, cyg_current_time() + 50)) //Acontece quando o comando anteria ainda nao foi totalmete processado (RX ainda nao deu post)
        {
          continue;
        }
        bool cmd_exec_ret = commands[i].cmd_fnct(argc, argv); //check for bad inputs

        if (!cmd_exec_ret) //If arguments were invalid
        {
          continue;
        }

        if ((i > 0) && (i < 12))
        {
          //Esperar pela mailBox do RX (TIMED_GET)
          m = (mBoxMessage *)cyg_mbox_timed_get(uiMboxH, cyg_current_time() + 50);

          if (m != NULL)
          {
            memcpy(bufr, (*m).data, (*m).cmd_dim);
            commands[i].cmd_display();
          }
          else
          {
            printf("Time to recieve message exceeded");
          }
        }
      }
      else
        printf("%s", InvalMsg);
    } /* if my_getline */
  }   /* forever */
}

void ui_th_prog(cyg_addrword_t data)
{
  printf("Working UI thread\n");

  cyg_mbox_create(&uiMboxH, &uiMbox);
  cyg_mbox_create(&TXMboxH, &TXMbox);

  cyg_semaphore_init(&newCmdSem, 1);

  cyg_mutex_init(&sharedBuffMutex);
  monitor();
}
