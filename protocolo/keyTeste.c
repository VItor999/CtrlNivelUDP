#include "kbhit.h"

int main(void)
{
  while(!kbhit())
    puts("Press a key!");
  printf("You pressed '%c'!\n", getchar());
  return 0;
}