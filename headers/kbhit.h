/**
*@file kbhit.h
*@author Lucas Esteves e Vitor Carvalho 
*@brief Biblioteca auxiliar definida pra a definição do kbhit no linux
* Bostejo completo aqui
*@version 0.1
*@date 2022-09-18
*
**/
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#ifndef kbhithead
/**
 * @brief Implementação do kbhit no UNIX
 * 
 * @return int retorna 1 se alguma tecla foi pressionada
 */
int kbhit(void)
{
  struct termios oldt, newt;
  int ch;
  int oldf;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);

  if(ch != EOF)
  {
    ungetc(ch, stdin);
    return 1;
  }

  return 0;
}
#define kbhithead
#endif
