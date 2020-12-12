#include <stdio.h>
#include <stdlib.h>
#include <cyg/io/io.h>
#include <cyg/kernel/kapi.h>

int periodOfTransference = 0;

unsigned char tempThreshold = 28;
unsigned char lumThreshold = 4;

cyg_alarm_t alarm_func;

//extern cyg_mutex_t sharedBuffMutex;

void proc_th_prog(cyg_addrword_t data)
{
    // cyg_handle_t counterH, system_clockH, alarmH;
    // cyg_tick_count_t ticks;
    // cyg_alarm alarm;

    // system_clockH = cyg_real_time_clock();
    // cyg_clock_to_counter(system_clockH, &counterH);

    // cyg_alarm_create(counterH, alarm_func,
    //                  (cyg_addrword_t)0,
    //                  &alarmH, &alarm);

    // cyg_alarm_initialize(alarmH, cyg_current_time() + 200, 200);

    // //Alarm
    // //Mailbox
    // printf("Working Processing thread\n");
}

void alarm_func(cyg_handle_t alarmH, cyg_addrword_t data)
{
}