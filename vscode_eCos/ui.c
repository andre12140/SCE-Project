/***************************************************************************
| File: monitor.c
|
| Autor: Carlos Almeida (IST), from work by Jose Rufino (IST/INESC), 
|        from an original by Leendert Van Doorn
| Data:  Nov 2002
***************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <cyg/io/io.h>
#include <cyg/kernel/kapi.h>

Cyg_ErrNo err;
cyg_io_handle_t serH;
cyg_mutex_t TX_mutex;

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

//Needs logic to control read and write
unsigned char bufw[20];
unsigned int w;

unsigned char bufr[20];

/*-------------------------------------------------------------------------+
| Header definition
+--------------------------------------------------------------------------*/
void cmd_sos(int argc, char **argv);
void cmd_sair(int argc, char **argv);
void cmd_ini(int argc, char **argv);

void cmd_status_display(void);

void cmd_rc(int argc, char **argv);
void cmd_rc_display(void);
void cmd_sc(int argc, char **argv);
void cmd_rtl(int argc, char **argv);
void cmd_rtl_display(void);
void cmd_rp(int argc, char **argv);
void cmd_rp_display(void);
void cmd_mmp(int argc, char **argv);
void cmd_mta(int argc, char **argv);
void cmd_ra(int argc, char **argv);
void cmd_ra_display(void);
void cmd_dac(int argc, char **argv);
void cmd_dtl(int argc, char **argv);
void cmd_aa(int argc, char **argv);

/*-------------------------------------------------------------------------+
| Variable and constants definition
+--------------------------------------------------------------------------*/
const char TitleMsg[] = "\n Application Control Monitor (tst)\n";
const char InvalMsg[] = "\nInvalid command!";

struct command_d
{
  void (*cmd_fnct)(int, char **);
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
    // {cmd_ir, "ir", "<p>              information about registers (NREG, nr, iread, iwrite)"},
    // {cmd_trc, "trc", "<n>              transfer n registers from current iread position"},
    // {cmd_tri, "tri", "<n> <i>          transfer n registers from index i (0 - oldest)"},

    // {cmd_irl, "irl", "                 information about local registers (NRBUF, nr, iread, iwrite)"},
    // {cmd_lr, "lr", "<n> <i>          list n registers (local memory) from index i (0 - oldest)"},
    // {cmd_dr, "dr", "                 delete registers (local memory)"},

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

///////////////////////////////////////////
// SOS TA MARADO

/*-------------------------------------------------------------------------+
| Function: cmd_sos - provides a rudimentary help
+--------------------------------------------------------------------------*/
void cmd_sos(int argc, char **argv)
{
  int i;

  printf("%s\n", TitleMsg);
  for (i = 0; i < NCOMMANDS; i++)
    printf("%s %s\n", commands[i].cmd_name, commands[i].cmd_help);
}

/*-------------------------------------------------------------------------+
| Function: cmd_sair - termina a aplicacao
+--------------------------------------------------------------------------*/
void cmd_sair(int argc, char **argv)
{
  //Falta terminar outras threads
  exit(0);
}

/*-------------------------------------------------------------------------+
| Function: cmd_ini - inicializar dispositivo
+--------------------------------------------------------------------------*/
void cmd_ini(int argc, char **argv)
{
  printf("io_lookup\n");
  if ((argc > 1) && (argv[1][0] = '1'))
    err = cyg_io_lookup("/dev/ser1", &serH);
  else
    err = cyg_io_lookup("/dev/ser0", &serH);
  printf("lookup err=%x\n", err);
}

/*-------------------------------------------------------------------------+
| Function: cmd_status_display - imprime resposta de funcoes de escrita
+--------------------------------------------------------------------------*/
void cmd_status_display()
{
  if (bufr[2] == CMD_OK)
  {
    printf("CMD_OK");
  }
  else if (bufr[2] == CMD_ERROR)
  {
    printf("CMD_ERROR");
  }
}

/*-------------------------------------------------------------------------+
| Function: cmd_rc - read clock
+--------------------------------------------------------------------------*/
void cmd_rc(int argc, char **argv)
{
  w = 3;
  bufw[2] = (unsigned char)EOM;
  bufw[1] = (unsigned char)RCLK;
  bufw[0] = (unsigned char)SOM;
}

void cmd_rc_display()
{
  printf("Horas = %d\n", bufr[2]);
  printf("Minutos = %d\n", bufr[3]);
  printf("Segundos = %d\n", bufr[4]);
}

/*-------------------------------------------------------------------------+
| Function: cmd_sc - set clock
+--------------------------------------------------------------------------*/
void cmd_sc(int argc, char **argv)
{
  if (argc != 4)
  {
    printf("Bad inputs\n");
    return;
  }
  unsigned int h, m, s;
  sscanf(argv[1], "%d", &h);
  sscanf(argv[2], "%d", &m);
  sscanf(argv[3], "%d", &s);

  //printf("Horas = %d\n", h);
  //printf("Minutos = %d\n", m);
  //printf("Segundos = %d\n", s);

  w = 6;
  bufw[5] = (unsigned char)EOM;
  bufw[4] = s;
  bufw[3] = m;
  bufw[2] = h;
  bufw[1] = (unsigned char)SCLK;
  bufw[0] = (unsigned char)SOM;
}

/*-------------------------------------------------------------------------+
| Function: cmd_rtl - read Temperature and Luminosity
+--------------------------------------------------------------------------*/
void cmd_rtl(int argc, char **argv)
{
  w = 3;
  bufw[2] = (unsigned char)EOM;
  bufw[1] = (unsigned char)RTL;
  bufw[0] = (unsigned char)SOM;
}

void cmd_rtl_display()
{
  printf("Temp = %d\n", bufr[2]);
  printf("Lum = %d\n", bufr[3]);
}

/*-------------------------------------------------------------------------+
| Function: cmd_rp - read parameters (PMON, TALA)
+--------------------------------------------------------------------------*/
void cmd_rp(int argc, char **argv)
{
  w = 3;
  bufw[2] = (unsigned char)EOM;
  bufw[1] = (unsigned char)RPAR;
  bufw[0] = (unsigned char)SOM;
}

void cmd_rp_display()
{
  printf("PMON = %d\n", bufr[2]);
  printf("TALA = %d\n", bufr[3]);
}

/*-------------------------------------------------------------------------+
| Function: cmd_mmp - modify monitoring period
+--------------------------------------------------------------------------*/
void cmd_mmp(int argc, char **argv)
{
  if (argc != 2)
  {
    printf("Bad inputs\n");
    return;
  }
  unsigned int p;
  sscanf(argv[1], "%d", &p);
  w = 4;
  bufw[3] = (unsigned char)EOM;
  bufw[2] = p;
  bufw[1] = (unsigned char)MMP;
  bufw[0] = (unsigned char)SOM;
}

/*-------------------------------------------------------------------------+
| Function: cmd_mta - modify time alarm (seconds)
+--------------------------------------------------------------------------*/
void cmd_mta(int argc, char **argv)
{
  if (argc != 2)
  {
    printf("Bad inputs\n");
    return;
  }
  unsigned int s;
  sscanf(argv[1], "%d", &s);
  w = 4;
  bufw[3] = (unsigned char)EOM;
  bufw[2] = s;
  bufw[1] = (unsigned char)MTA;
  bufw[0] = (unsigned char)SOM;
}

/*-------------------------------------------------------------------------+
| Function: cmd_ra - read Alarm values
+--------------------------------------------------------------------------*/
void cmd_ra(int argc, char **argv)
{
  w = 3;
  bufw[2] = (unsigned char)EOM;
  bufw[1] = (unsigned char)RALA;
  bufw[0] = (unsigned char)SOM;
}

void cmd_ra_display()
{
  printf("Clock Alarm = %02d : %02d : %02d\n", bufr[2], bufr[3], bufr[4]);
  printf("Temp Alarm = %d\n", bufr[5]);
  printf("Lum Alarm = %d\n", bufr[6]);
  printf("Alarm State = %d\n", bufr[7]);
}

/*-------------------------------------------------------------------------+
| Function: cmd_dac - define alarm clock
+--------------------------------------------------------------------------*/
void cmd_dac(int argc, char **argv)
{
  if (argc != 4)
  {
    printf("Bad inputs\n");
    return;
  }
  unsigned int h, m, s;
  sscanf(argv[1], "%d", &h);
  sscanf(argv[2], "%d", &m);
  sscanf(argv[3], "%d", &s);

  //printf("Horas = %d\n", h);
  //printf("Minutos = %d\n", m);
  //printf("Segundos = %d\n", s);

  w = 6;
  bufw[5] = (unsigned char)EOM;
  bufw[4] = s;
  bufw[3] = m;
  bufw[2] = h;
  bufw[1] = (unsigned char)DAC;
  bufw[0] = (unsigned char)SOM;
}

/*-------------------------------------------------------------------------+
| Function: cmd_dtl - define Temperature and Luminosity Alarm
+--------------------------------------------------------------------------*/
void cmd_dtl(int argc, char **argv)
{
  if (argc != 3)
  {
    printf("Bad inputs\n");
    return;
  }
  unsigned int T, L;
  sscanf(argv[1], "%d", &T);
  sscanf(argv[2], "%d", &L);

  w = 5;
  bufw[4] = (unsigned char)EOM;
  bufw[3] = L;
  bufw[2] = T;
  bufw[1] = (unsigned char)DATL;
  bufw[0] = (unsigned char)SOM;
}

/*-------------------------------------------------------------------------+
| Function: cmd_aa - activate/deactivate alarms (1/0)
+--------------------------------------------------------------------------*/
void cmd_aa(int argc, char **argv)
{
  if (argc != 2)
  {
    printf("Bad inputs\n");
    return;
  }
  unsigned int a;
  sscanf(argv[1], "%d", &a);
  w = 4;
  bufw[3] = (unsigned char)EOM;
  bufw[2] = a;
  bufw[1] = (unsigned char)AALA;
  bufw[0] = (unsigned char)SOM;
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
void monitor(void)
{
  static char *argv[ARGVECSIZE + 1], *p;
  int argc, i;

  printf("%s Type sos for help\n", TitleMsg);
  for (;;) //Espera activa WARNING
  {
    cyg_thread_delay(20);
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
        commands[i].cmd_fnct(argc, argv);
        //Wait for response
        cyg_thread_delay(100);
        if ((strcmp(argv[0], "sos") != 0) && (strcmp(argv[0], "ini") != 0))
        {
          commands[i].cmd_display();
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
  cyg_mutex_init(&TX_mutex);
  monitor();
}
