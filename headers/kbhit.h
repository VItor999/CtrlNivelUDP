/**
*@file kbhit.h
*@author Lucas Esteves e Vitor Carvalho 
*@brief Biblioteca auxiliar definida pra a definição do kbhit no linux
* 
*@version 0.1
*@date 2022-09-18
*
**/

//===================== Bibliotecas utilizadas =====================//
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#ifndef kbhithead
//===================== Cabeçalhos de Funções =====================//
int kbhit(void);
char teclado();

//#################################################################//
//########################   FUNÇÕES   ############################//
//#################################################################//

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

/**
*@brief captura uma tecla qualquer, se o teclado for pressionado
*
*@return char retonar 'e' ou a tecla pressionada
**/
char teclado(){
  char r ='e';
  // bloco para captura de tecla
  if (kbhit()){ // LINUX não tem um kbhit() como o windows -> ver arquivo kbhit.h
    r = getchar();
  } 
  return r;
}
#define kbhithead
#endif
