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

#define uiID 0xCE
#define procID 0xCF

#define CPT 0xD0
#define MPT 0xD1
#define CTTL 0xD2
#define DTTL 0xD3
#define PR 0xD4

#define NRBUF 100
#define REGDIM 5

typedef struct mBoxMessages
{
    unsigned char data[20]; //Data to be transmited
    unsigned int cmd_dim;   //Number of bytes to be transmited
} mBoxMessage;

extern cyg_sem_t newCmdSem;

extern cyg_mutex_t sharedBuffMutex;

cyg_mbox procMbox;
cyg_handle_t procMboxH;
extern cyg_handle_t TXMboxH;

cyg_alarm_t alarm_func;
cyg_handle_t alarmH;

int periodOfTransference = 1;

unsigned int tempThreshold = 28;
unsigned int lumThreshold = 4;

extern unsigned char eCosRingBuff[NRBUF][REGDIM];
extern int iwrite;
extern int iread;
extern int nr;

mBoxMessage alarmCmd; //Comand that is requested periodicaly
mBoxMessage m;

void analyseRegisters(void)
{
    int maxReadings = (iwrite - iread);
    if (maxReadings <= 0)
    {
        maxReadings = iwrite + (NRBUF - iread);
    }
    int i;
    cyg_mutex_lock(&sharedBuffMutex);
    for (i = 0; i < maxReadings; i++)
    {
        if (eCosRingBuff[iread][3] > tempThreshold || eCosRingBuff[iread][4] < lumThreshold)
        {
            printf("Clock = %02d : %02d : %02d\n", eCosRingBuff[iread][0], eCosRingBuff[iread][1], eCosRingBuff[iread][2]);
            printf("Temp = %d\n", eCosRingBuff[iread][3]);
            printf("Lum = %d\n\n", eCosRingBuff[iread][4]);
        }
        iread++;
        if (iread > NRBUF - 1)
        {
            iread = 0;
        }
    }
    cyg_mutex_unlock(&sharedBuffMutex);
}

void proc_cpt(void)
{
    printf("Period of transference = %d\n", periodOfTransference);
}

void proc_mpt(void)
{
    periodOfTransference = m.data[2];
    if (periodOfTransference == 0)
    {
        cyg_alarm_disable(alarmH);
    }
    else
    {
        cyg_alarm_initialize(alarmH, cyg_current_time() + (periodOfTransference * 1000), (periodOfTransference * 1000));
    }
    printf("CMD_OK\n");
}

void proc_cttl(void)
{
    printf("eCos Temp threshold = %d\n", tempThreshold);
    printf("eCos Lum threshold = %d\n", lumThreshold);
}

void proc_dttl(void)
{
    if ((m.data[2] >= 0 && m.data[2] < 50) && (m.data[3] >= 0 && m.data[3] < 8))
    {
        tempThreshold = m.data[2];
        lumThreshold = m.data[3];
        printf("CMD_OK\n");
    }
    else
    {
        printf("Values for temperature or luminosity out of range\n");
        printf("CMD_ERROR\n");
    }
}

int t1Index, t2Index;
void searchTimes(void)
{
    bool foundOneFlag = true;
    if (m.cmd_dim == 9) //Tem t1 e t2
    {
        foundOneFlag = false;
    }

    int i;
    for (i = 0; i < nr; i++)
    {
        if (eCosRingBuff[i][0] == m.data[2] && eCosRingBuff[i][0] == m.data[3] && eCosRingBuff[i][0] == m.data[4])
        {
            t1Index = i;
            if (foundOneFlag)
            {
                break;
            }
            else
            {
                foundOneFlag = true;
            }
        }
        if ((m.cmd_dim == 9) && (eCosRingBuff[i][0] == m.data[2] && eCosRingBuff[i][0] == m.data[3] && eCosRingBuff[i][0] == m.data[4])) //Tem t1 e t2
        {
            t2Index = i;
            if (foundOneFlag)
            {
                break;
            }
            else
            {
                foundOneFlag = true;
            }
        }
    }
}

void calcMaxMinMean(int firstIndex, int secondIndex)
{
}

void proc_pr(void)
{
    t1Index = -1;
    t2Index = -1;
    //Ordenar um buffer por tempo????????????????
    searchTimes();
    if (t1Index == -1) //Nao encontrou o tempo t1 logo sao os registos todos
    {
        calcMaxMinMean(0, nr);
    }
    else if (t2Index == -1) //Encontrou o tempo t1 mas não o t2 logo sao os resgisto a partir de t1 ate ao final
    {
        calcMaxMinMean(t1Index, nr);
    }
    else //Encontrou t1 e t2 logo sao os registos entre t1 e t2
    {
        calcMaxMinMean(t1Index, t2Index); //Se os indices nao estiverem por ordem isto nao funciona
    }
}

void proc_th_main(void)
{
    while (1)
    {
        m = *(mBoxMessage *)cyg_mbox_get(procMboxH); //Recebe mails do Rx e do UI
        if (m.data[m.cmd_dim] == procID)             //Mensagem vinda de RX em que foi criada pela processing task
        {
            if (m.data[1] == TRGC && m.data[2] == CMD_OK)
            {
                analyseRegisters();
            }
            else //Mensagem asyncrona do PIC
            {
            }
        }
        else if (m.data[m.cmd_dim] == uiID) //Mensagem vinda do UI
        {
            if (m.data[1] == CPT)
            {
                proc_cpt();
            }
            else if (m.data[1] == MPT)
            {
                proc_mpt();
            }
            else if (m.data[1] == CTTL)
            {
                proc_cttl();
            }
            else if (m.data[1] == DTTL)
            {
                proc_dttl();
            }
            else if (m.data[1] == PR)
            {
                proc_pr();
            }
        }
    }
}

void proc_th_prog(cyg_addrword_t data)
{
    printf("Working Processing thread\n");

    alarmCmd.data[0] = SOM;
    alarmCmd.data[1] = TRGC;
    alarmCmd.data[2] = 1;
    alarmCmd.data[3] = EOM;
    alarmCmd.data[4] = procID;
    alarmCmd.cmd_dim = 4;

    cyg_mbox_create(&procMboxH, &procMbox);

    cyg_handle_t counterH, system_clockH;
    cyg_alarm alarm;

    system_clockH = cyg_real_time_clock();
    cyg_clock_to_counter(system_clockH, &counterH);

    cyg_alarm_create(counterH, alarm_func,
                     (cyg_addrword_t)0,
                     &alarmH, &alarm);

    /////////////////////////////////////////////// Penso que nao esta com o periodo correto /////////////////////////////////////////////////
    cyg_alarm_initialize(alarmH, cyg_current_time() + (periodOfTransference * 1000), (periodOfTransference * 1000));

    proc_th_main();
}

void alarm_func(cyg_handle_t alarmH, cyg_addrword_t data)
{
    if (!cyg_semaphore_timed_wait(&newCmdSem, cyg_current_time() + 50)) //Acontece quando o comando anterior ainda nao foi totalmete processado (RX ainda nao deu post)
    {
        return;
    }
    if (!cyg_mbox_tryput(TXMboxH, &alarmCmd))
    {
        printf("Something went wrong with sending mail box to TX from alarm_func (Processing thread)");
    }
}