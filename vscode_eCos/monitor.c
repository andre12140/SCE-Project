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

/*-------------------------------------------------------------------------+
| Headers of command functions
+--------------------------------------------------------------------------*/
extern void cmd_sair(int, char **);
extern void cmd_ini(int, char **);
extern void cmd_ems(int, char **);
extern void cmd_emh(int, char **);
extern void cmd_rms(int, char **);
extern void cmd_rmh(int, char **);
extern void cmd_test(int, char **);
void cmd_sos(int, char **);

extern void cmd_rc(int, char **);

/*-------------------------------------------------------------------------+
| Variable and constants definition
+--------------------------------------------------------------------------*/
const char TitleMsg[] = "\n Application Control Monitor (tst)\n";
const char InvalMsg[] = "\nInvalid command!";

struct command_d
{
  void (*cmd_fnct)(int, char **);
  char *cmd_name;
  char *cmd_help;
}

const commands[] = {
    {cmd_sos, "sos", "                 help"},

    {cmd_rc, "rc", "                 read clock"},
    // {cmd_sc, "sc", "                 set clock"},
    // {cmd_rtl, "rtl", "                 read temperature and luminosity"},
    // {cmd_rp, "rp", "                 read parameters (PMON, TALA)"},
    // {cmd_mmp, "mmp", "<p>              modify monitoring period (seconds - 0 deactivate)"},
    // {cmd_mta, "mta", "<t>              modify time alarm (seconds)"},
    // {cmd_ra, "ra", "                 read alarms (clock, temperature, luminosity, active/inactive-1/0)"},
    // {cmd_dac, "dac", "<h> <m> <s>      define alarm clock"},
    // {cmd_dtl, "dtl", "<t> <l>          define alarm temperature and luminosity"},
    // {cmd_aa, "aa", "<a>              activate/deactivate alarms (1/0)"},
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

    {cmd_sair, "sair", "                sair"},
    {cmd_ini, "ini", "<d>              inicializar dispositivo (0/1) ser0/ser1"},
    {cmd_ems, "ems", "<str>            enviar mensagem (string)"},
    {cmd_emh, "emh", "<h1> <h2> <...>  enviar mensagem (hexadecimal)"},
    {cmd_rms, "rms", "<n>              receber mensagem (string)"},
    {cmd_rmh, "rmh", "<n>              receber mensagem (hexadecimal)"},
    {cmd_test, "teste", "<arg1> <arg2>  comando de teste"}};

#define NCOMMANDS (sizeof(commands) / sizeof(struct command_d))
#define ARGVECSIZE 10
#define MAX_LINE 50

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
| Function: monitor        (called from main) 
+--------------------------------------------------------------------------*/
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
        commands[i].cmd_fnct(argc, argv);
      else
        printf("%s", InvalMsg);
    } /* if my_getline */
  }   /* forever */
}
