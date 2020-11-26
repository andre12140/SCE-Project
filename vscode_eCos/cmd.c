#include <stdio.h>

extern void cmd_ini(int, char **);
extern void monitor(void);

int main(void)
{
  cmd_ini(0, NULL);
  monitor();

  return 0;
}
