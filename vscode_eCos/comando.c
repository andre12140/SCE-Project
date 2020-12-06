/***************************************************************************
| File: comando.c  -  Concretizacao de comandos (exemplo)
|
| Autor: Carlos Almeida (IST)
| Data:  Maio 2008
***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <cyg/io/io.h>

Cyg_ErrNo err;
cyg_io_handle_t serH;

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

/*-------------------------------------------------------------------------+
| Function: cmd_sair - termina a aplicacao
+--------------------------------------------------------------------------*/
void cmd_sair(int argc, char **argv)
{
  exit(0);
}

/*-------------------------------------------------------------------------+
| Function: cmd_test - apenas como exemplo
+--------------------------------------------------------------------------*/
void cmd_test(int argc, char **argv)
{
  int i;

  /* exemplo -- escreve argumentos */
  for (i = 0; i < argc; i++)
    printf("\nargv[%d] = %s", i, argv[i]);
}

/*-------------------------------------------------------------------------+
| Function: cmd_ems - enviar mensagem (string)
+--------------------------------------------------------------------------*/
void cmd_ems(int argc, char **argv)
{
  unsigned int n;

  if (argc > 1)
  {
    n = strlen(argv[1]) + 1;
    err = cyg_io_write(serH, argv[1], &n);
    printf("io_write err=%x, n=%d str=%s\n", err, n, argv[1]);
  }
  else
  {
    n = 10;
    err = cyg_io_write(serH, "123456789", &n);
    printf("io_write err=%x, n=%d str=%s\n", err, n, "123456789");
  }
}

/*-------------------------------------------------------------------------+
| Function: cmd_emh - enviar mensagem (hexadecimal)
+--------------------------------------------------------------------------*/
void cmd_emh(int argc, char **argv)
{
  unsigned int n, i;
  unsigned char bufw[50];

  if ((n = argc) > 1)
  {
    n--;
    if (n > 50)
      n = 50;
    for (i = 0; i < n; i++)
    //      sscanf(argv[i+1], "%hhx", &bufw[i]);
    {
      unsigned int x;
      sscanf(argv[i + 1], "%x", &x);
      bufw[i] = (unsigned char)x;
    }
    err = cyg_io_write(serH, bufw, &n);
    printf("io_write err=%x, n=%d\n", err, n);
    for (i = 0; i < n; i++)
      printf("buf[%d]=%x\n", i, bufw[i]);
  }
  else
  {
    printf("nenhuma mensagem!!!\n");
  }
}

/*-------------------------------------------------------------------------+
| Function: cmd_rms - receber mensagem (string)
+--------------------------------------------------------------------------*/
void cmd_rms(int argc, char **argv)
{
  unsigned int n;
  char bufr[50];

  if (argc > 1)
    n = atoi(argv[1]);
  if (n > 50)
    n = 50;
  err = cyg_io_read(serH, bufr, &n);
  printf("io_read err=%x, n=%d buf=%s\n", err, n, bufr);
}

/*-------------------------------------------------------------------------+
| Function: cmd_rmh - receber mensagem (hexadecimal)
+--------------------------------------------------------------------------*/
void cmd_rmh(int argc, char **argv)
{
  unsigned int n, i;
  unsigned char bufr[50];

  if (argc > 1)
    n = atoi(argv[1]);
  if (n > 50)
    n = 50;
  err = cyg_io_read(serH, bufr, &n);
  printf("io_read err=%x, n=%d\n", err, n);
  for (i = 0; i < n; i++)
    printf("buf[%d]=%x\n", i, bufr[i]);
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
| Function: cmd_rc - read clock
+--------------------------------------------------------------------------*/
void cmd_rc(int argc, char **argv)
{

  unsigned char bufw[3];
  unsigned int w = sizeof(bufw) + 1;

  unsigned int r = 6;
  unsigned char bufr[r];

  bufw[0] = (unsigned char)SOM;
  bufw[1] = (unsigned char)RCLK;
  bufw[2] = (unsigned char)EOM;

  printf("bufw = %x, %x, %x\n", bufw[0], bufw[1], bufw[2]);

  err = cyg_io_write(serH, bufw, &w);
  printf("io_write err=%x, n=%d\n", err, w);

  err = cyg_io_read(serH, bufr, &r);
  printf("bufr = %hhx\n", bufr[0]);
  if (bufr[1] == RCLK)
  {
    printf("FUNCIONA");
  }
  printf("bufr = %hhx\n", bufr[1]);
  printf("bufr = %hhx\n", bufr[2]);
  printf("bufr = %hhx\n", bufr[3]);
  printf("bufr = %hhx\n", bufr[4]);
  printf("bufr = %hhx\n", bufr[5]);

  printf("BUFR = %s\n", bufr);

  printf("io_read err=%x, n=%d\n", err, r);
}