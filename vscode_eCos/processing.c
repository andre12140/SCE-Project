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

struct Time
{
    int h;
    int m;
    int s;
};

extern cyg_sem_t newCmdSem;

extern cyg_mutex_t sharedBuffMutex;
extern cyg_mutex_t printfMutex;

cyg_mbox procMbox;
cyg_handle_t procMboxH;
extern cyg_handle_t TXMboxH;
extern cyg_handle_t uiMboxH;

cyg_alarm_t alarm_func;
cyg_handle_t alarmH;

unsigned int periodOfTransference = 0;

unsigned int tempThreshold = 28;
unsigned int lumThreshold = 4;

extern unsigned char eCosRingBuff[NRBUF][REGDIM];
extern int iwrite;
extern int iread;
extern int nr;

bool sentMessageFlag = false;

mBoxMessage alarmCmd; //Comando que é solicitado periodicamente
mBoxMessage *m;       //Mensagem lida
mBoxMessage m_r;      //Mensagem de leitura (copia de m)
mBoxMessage m_w;      //Mensagem de escrita para o UI

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
            cyg_mutex_lock(&printfMutex);
            printf("Clock = %02d : %02d : %02d\n", eCosRingBuff[iread][0], eCosRingBuff[iread][1], eCosRingBuff[iread][2]);
            printf("Temp = %d\n", eCosRingBuff[iread][3]);
            printf("Lum = %d\n\n", eCosRingBuff[iread][4]);
            printf("Cmd> ");
            cyg_mutex_unlock(&printfMutex);
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
    m_w.cmd_dim = (unsigned int)4;
    unsigned char bufw[m_w.cmd_dim + 1];
    bufw[4] = (unsigned char)procID;
    bufw[3] = (unsigned char)EOM;
    bufw[2] = (unsigned char)periodOfTransference;
    bufw[1] = (unsigned char)CPT;
    bufw[0] = (unsigned char)SOM;
    memcpy(m_w.data, bufw, m_w.cmd_dim + 1);
    cyg_mbox_tryput(uiMboxH, &m_w);
}

void proc_mpt(void)
{
    periodOfTransference = m_r.data[2];
    if (periodOfTransference == 0)
    {
        cyg_alarm_disable(alarmH);
    }
    else
    {
        cyg_alarm_initialize(alarmH, cyg_current_time() + (periodOfTransference * 100), (periodOfTransference * 100));
    }

    m_w.cmd_dim = 4;
    unsigned char bufw[m_w.cmd_dim + 1];
    bufw[4] = (unsigned char)procID;
    bufw[3] = (unsigned char)EOM;
    bufw[2] = (unsigned char)CMD_OK;
    bufw[1] = (unsigned char)MPT;
    bufw[0] = (unsigned char)SOM;
    memcpy(m_w.data, bufw, m_w.cmd_dim + 1);
    cyg_mbox_tryput(uiMboxH, &m_w);
}

void proc_cttl(void)
{
    m_w.cmd_dim = 5;
    unsigned char bufw[m_w.cmd_dim + 1];
    bufw[5] = (unsigned char)procID;
    bufw[4] = (unsigned char)EOM;
    bufw[3] = lumThreshold;
    bufw[2] = tempThreshold;
    bufw[1] = (unsigned char)CTTL;
    bufw[0] = (unsigned char)SOM;
    memcpy(m_w.data, bufw, m_w.cmd_dim + 1);
    cyg_mbox_tryput(uiMboxH, &m_w);
}

void proc_dttl(void)
{
    m_w.cmd_dim = 4;
    unsigned char bufw[m_w.cmd_dim + 1];
    bufw[4] = (unsigned char)procID;
    bufw[3] = (unsigned char)EOM;

    bufw[1] = (unsigned char)DTTL;
    bufw[0] = (unsigned char)SOM;

    if ((m_r.data[2] >= 0 && m_r.data[2] < 50) && (m_r.data[3] >= 0 && m_r.data[3] < 8))
    {
        tempThreshold = m_r.data[2];
        lumThreshold = m_r.data[3];
        bufw[2] = (unsigned char)CMD_OK;
    }
    else
    {
        bufw[2] = (unsigned char)CMD_ERROR;
    }
    memcpy(m_w.data, bufw, m_w.cmd_dim + 1);
    cyg_mbox_tryput(uiMboxH, &m_w);
}

unsigned int maxTemp;
unsigned int minTemp;
unsigned int meanTemp;

unsigned int maxLum;
unsigned int minLum;
unsigned int meanLum;

void calcAux(int i)
{
    meanTemp += eCosRingBuff[i][3];
    meanLum += eCosRingBuff[i][4];

    if (maxTemp == -1 && minTemp == -1 && maxLum == -1 && minLum == -1)
    {
        maxTemp = eCosRingBuff[i][3];
        minTemp = eCosRingBuff[i][3];

        maxLum = eCosRingBuff[i][4];
        minLum = eCosRingBuff[i][4];
    }
    // printf("Register Temp = %d\n", eCosRingBuff[i][3]);
    // printf("Register Lum = %d\n", eCosRingBuff[i][4]);

    if (eCosRingBuff[i][3] > maxTemp)
    {
        maxTemp = eCosRingBuff[i][3];
    }
    if (eCosRingBuff[i][3] < minTemp)
    {
        minTemp = eCosRingBuff[i][3];
    }

    if (eCosRingBuff[i][4] > maxLum)
    {
        maxLum = eCosRingBuff[i][4];
    }
    if (eCosRingBuff[i][4] < minLum)
    {
        minLum = eCosRingBuff[i][4];
    }
}

void calcMaxMinMean(int firstIndex, int secondIndex)
{
    maxTemp = -1;
    minTemp = -1;
    meanTemp = 0;

    maxLum = -1;
    minLum = -1;
    meanLum = 0;

    int i;
    if (firstIndex > secondIndex) //Ring buffer turned arround
    {
        for (i = firstIndex; i < NRBUF; i++)
        {
            calcAux(i);
        }
        for (i = 0; i <= secondIndex; i++)
        {
            calcAux(i);
        }

        meanTemp = meanTemp / (secondIndex + 1 + (NRBUF - firstIndex));
        meanLum = meanLum / (secondIndex + 1 + (NRBUF - firstIndex));
        return;
    }

    for (i = firstIndex; i <= secondIndex; i++)
    {
        calcAux(i);
    }
    meanTemp = meanTemp / (secondIndex + 1 - firstIndex);
    meanLum = meanLum / (secondIndex + 1 - firstIndex);
}

// Computes difference between time periods
void differenceBetweenTimePeriod(struct Time start,
                                 struct Time stop,
                                 struct Time *diff)
{
    while (stop.s > start.s)
    {
        --start.m;
        start.s += 60;
    }
    diff->s = start.s - stop.s;
    while (stop.m > start.m)
    {
        --start.h;
        start.m += 60;
    }
    diff->m = start.m - stop.m;
    diff->h = start.h - stop.h;
}

//////////////////////////Dimensao de unsigned int nao chega para guardar um tempo todo em segundos//////////////////////////////////
unsigned int getTimeindex(struct Time t)
{
    struct Time arrayTime, diff;
    unsigned int arrayDiffseconds, aux = 0;
    int index = -1;
    int i;
    for (i = 0; i < nr; i++)
    {
        arrayTime.h = eCosRingBuff[i][0];
        arrayTime.m = eCosRingBuff[i][1];
        arrayTime.s = eCosRingBuff[i][2];
        differenceBetweenTimePeriod(t, arrayTime, &diff);
        arrayDiffseconds = abs(diff.s) + (abs(diff.m) * 60) + (abs(diff.h) * 3600);
        if (index == -1)
        {
            aux = arrayDiffseconds;
            index = i;
        }
        if (arrayDiffseconds < aux)
        {
            aux = arrayDiffseconds;
            index = i;
        }
    }
    return index;
}

void proc_pr(void)
{
    unsigned int t1Index;
    unsigned int t2Index;
    if (m_r.cmd_dim == 3)
    {
        t1Index = 0;
        t2Index = nr - 1;
    }
    else if (m_r.cmd_dim == 6)
    {
        struct Time t1 = {m_r.data[2], m_r.data[3], m_r.data[4]};
        t1Index = getTimeindex(t1);
        t2Index = nr - 1;
    }
    else if (m_r.cmd_dim == 9)
    {
        struct Time t1 = {m_r.data[2], m_r.data[3], m_r.data[4]};
        struct Time t2 = {m_r.data[5], m_r.data[6], m_r.data[7]};
        struct Time diff;
        differenceBetweenTimePeriod(t2, t1, &diff);
        if (diff.h < 0 || diff.m < 0 || diff.s < 0) //t1 maior que t2
        {
            m_w.cmd_dim = 4;
            unsigned char bufw[m_w.cmd_dim + 1];
            bufw[4] = (unsigned char)procID;
            bufw[3] = (unsigned char)EOM;
            bufw[2] = (unsigned char)CMD_ERROR;
            bufw[1] = (unsigned char)PR;
            bufw[0] = (unsigned char)SOM;
            memcpy(m_w.data, bufw, m_w.cmd_dim + 1);
            cyg_mbox_tryput(uiMboxH, &m_w);
            return;
        }

        t1Index = getTimeindex(t1);
        t2Index = getTimeindex(t2);
    }
    else //ERRO
    {
        m_w.cmd_dim = 4;
        unsigned char bufw[m_w.cmd_dim + 1];
        bufw[4] = (unsigned char)procID;
        bufw[3] = (unsigned char)EOM;
        bufw[2] = (unsigned char)CMD_ERROR;
        bufw[1] = (unsigned char)PR;
        bufw[0] = (unsigned char)SOM;
        memcpy(m_w.data, bufw, m_w.cmd_dim + 1);
        cyg_mbox_tryput(uiMboxH, &m_w);
        return;
    }

    calcMaxMinMean(t1Index, t2Index);

    // printf("Temperature: Max = %d, Min = %d, Mean = %d\n", maxTemp, minTemp, meanTemp);
    // printf("Luminosity: Max = %d, Min = %d, Mean = %d\n", maxLum, minLum, meanLum);

    m_w.cmd_dim = 9;
    unsigned char bufw[m_w.cmd_dim + 1];
    bufw[9] = (unsigned char)procID;
    bufw[8] = (unsigned char)EOM;
    bufw[7] = (unsigned char)meanLum;
    bufw[6] = (unsigned char)minLum;
    bufw[5] = (unsigned char)maxLum;
    bufw[4] = (unsigned char)meanTemp;
    bufw[3] = (unsigned char)minTemp;
    bufw[2] = (unsigned char)maxTemp;
    bufw[1] = (unsigned char)MPT;
    bufw[0] = (unsigned char)SOM;
    memcpy(m_w.data, bufw, m_w.cmd_dim + 1);
    cyg_mbox_tryput(uiMboxH, &m_w);
}

void asyncMessage(void)
{
    cyg_mutex_lock(&printfMutex);
    printf("Buffer in PIC is half full, starting periodic transference if not yet started\n");
    cyg_mutex_unlock(&printfMutex);

    if (periodOfTransference == 0)
    {
        periodOfTransference = 1;
        cyg_alarm_initialize(alarmH, cyg_current_time() + (periodOfTransference * 100), (periodOfTransference * 100));
    }
}

void proc_th_main(void)
{
    while (1)
    {
        m = (mBoxMessage *)cyg_mbox_timed_get(procMboxH, cyg_current_time() + 10); //100ms

        if (m == NULL)
        {
            if (sentMessageFlag)
            {
                sentMessageFlag = false;
                cyg_semaphore_post(&newCmdSem); //post sem
            }
            continue;
        }

        memcpy(&m_r, m, sizeof(mBoxMessage));

        if (!(m_r.data[2] == NMFL))
        {
            cyg_semaphore_post(&newCmdSem); //post sem
        }

        if (m_r.data[m_r.cmd_dim] == procID) //Mensagem vinda de RX em que foi criada pela processing task
        {
            sentMessageFlag = false;
            if (m_r.data[1] == TRGC && m_r.data[2] == CMD_OK)
            {
                analyseRegisters();
            }
            else //Mensagem assincrona do PIC
            {
                asyncMessage();
            }
        }
        else if (m_r.data[m_r.cmd_dim] == uiID) //Mensagem vinda do UI
        {
            if (m_r.data[1] == CPT)
            {
                proc_cpt();
            }
            else if (m_r.data[1] == MPT)
            {
                proc_mpt();
            }
            else if (m_r.data[1] == CTTL)
            {
                proc_cttl();
            }
            else if (m_r.data[1] == DTTL)
            {
                proc_dttl();
            }
            else if (m_r.data[1] == PR)
            {
                proc_pr();
            }
        }
    }
}

void proc_th_prog(cyg_addrword_t data)
{

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

    //Cada tick demora 10ms
    //cyg_alarm_initialize(alarmH, cyg_current_time() + (periodOfTransference * 300), (periodOfTransference * 100));

    cyg_mutex_lock(&printfMutex);
    printf("Working Processing thread\n");
    cyg_mutex_unlock(&printfMutex);

    proc_th_main();
}

//VERIFICAR SE O SEMAFORO NAO ENTRA EM PARAFUSO
void alarm_func(cyg_handle_t alarmH, cyg_addrword_t data)
{
    //printf("IN ALARM\n");
    if (cyg_semaphore_trywait(&newCmdSem))
    {
        if (!cyg_mbox_tryput(TXMboxH, &alarmCmd))
        {
            printf("Something went wrong with sending mail box to TX from alarm_func (Processing thread)\n");
        }
        else
        {
            sentMessageFlag = true;
        }
    }
    else
    {
        printf("Can't aquire register because RX is waiting to get last message (In Alarm)\n");
    }
}