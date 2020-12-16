#include <stdio.h>
#include <cyg/kernel/kapi.h>

extern void cmd_ini(int, char **);
extern void monitor(void);

cyg_thread thread_s[4]; /* space for two thread objects */

char stack[4][4096]; /* space for two 4K stacks */

/* now the handles for the threads */
cyg_handle_t ui_thread, tx_thread, rx_thread, proc_thread;

/* and now variables for the procedure which is the thread */
cyg_thread_entry_t ui_th_prog, tx_th_prog, rx_th_prog, proc_th_prog;

extern void ui_th_prog(cyg_addrword_t);
extern void tx_th_prog(cyg_addrword_t);
extern void rx_th_prog(cyg_addrword_t);
extern void proc_th_prog(cyg_addrword_t);

int main(void)
{
  cmd_ini(0, NULL);

  //Main thread com a prioridade mais alta para conseguir inicializar as threads todas
  cyg_thread_create(11, ui_th_prog, (cyg_addrword_t)0,
                    "UI", (void *)stack[0], 4096,
                    &ui_thread, &thread_s[0]);

  cyg_thread_create(10, tx_th_prog, (cyg_addrword_t)0,
                    "TX", (void *)stack[1], 4096,
                    &tx_thread, &thread_s[1]);

  cyg_thread_create(10, rx_th_prog, (cyg_addrword_t)0,
                    "RX", (void *)stack[2], 4096,
                    &rx_thread, &thread_s[2]);

  cyg_thread_create(10, proc_th_prog, (cyg_addrword_t)0,
                    "PROC", (void *)stack[3], 4096,
                    &proc_thread, &thread_s[3]);

  cyg_thread_resume(ui_thread);
  cyg_thread_resume(tx_thread);
  cyg_thread_resume(rx_thread);
  cyg_thread_resume(proc_thread);

  printf("End of main thread\n");

  return 0;
}
