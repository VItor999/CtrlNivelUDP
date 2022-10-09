/**
*@file serverteste.c
*@author Lucas Esteves e Vitor Carvalho
*@brief Código para teste manual do protocolo, pode emular respostas de um cliente ou servidor. 
*@version 0.1
*@date 2022-09-21
*
**/

//===================== Bibliotecas utilizadas =====================//

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <math.h>
#include "../headers/kbhit.h"
#include "../headers/protocolo.h"


//====================== Definições efetuadas ======================//
#define BUFFER_SIZE 1024


//===================== Cabeçalhos de Funções =====================//
void error(const char *msg);


//#################################################################//
//#########################    MAIN    ############################//
//#################################################################//

int main(int argc, char *argv[])
{
  // lembrando que: argc = total de argumentos da chamada
  //               *argv = argumentos recebidos (OBS: 0 = chamada do program, 1 - n o resto)
  // Argumentos recebibos sempre em array de char (string)
  // Só preciso passar a porta no servidor
  int sock, length, fromlen, n;
  struct sockaddr_in server;
  struct sockaddr_in from;
  char buffer[BUFFER_SIZE];
  char out = '\0'; // tecla em que irei guardar a saida
  char retorno [11] ="Comando   ";
  int isserver;
  TPMENSAGEM mensagem;
  system("clear");
  if (argc < 2 && argc!=3)  // chamada de programa + porta
  {
    fprintf(stderr, "usage %s port isserver\n",argv[0]);
    exit(0);
  }
  else if (argc==2) { // se passar direto simulo servidor
    isserver = 1;
  }
  else{
    isserver = atoi(argv[2]);
  }
  
  sock = socket(AF_INET, SOCK_DGRAM, 0); // SOCK_STREAM -> TCP/IP
  fcntl(sock, F_SETFL, O_NONBLOCK);
  if (sock < 0)
  {
    error("Opening socket");
  }
  length = sizeof(server);
  bzero(&server, length); // limpando os valores do server
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(atoi(argv[1])); // converte o valor da porta para o formato necessário
  // Binding
  if (bind(sock, (struct sockaddr *)&server, length) < 0)
  {
    error("binding");
  }
  fromlen = sizeof(struct sockaddr_in);
  // recvfrom recebe algo sendto envia
  if (isserver){
    printf("Servidor iniciado para teste do servidor, parabens\n");
  }
   else{
    printf("Servidor iniciado para teste do cliente, parabens\n");
  }
  while (out != 27 && buffer[0] != '\n') // pressionando esq encerro o server
  {
    // bloco para captura de tecla
    if (kbhit())
    { // LINUX não tem um kbhit() como o windows -> ver arquivo kbhit.h
      out = getchar();
    }
    // de onde vem, onde escrevo, tamanho, argumento padrao, estrutura e tamanho
	bzero(buffer, BUFFER_SIZE);
    n = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&from, &fromlen);
    if (n < 0 && n != -1)
    {
      error("Receber");
    }
    if (n > 0)
    {
      char *tk;
      char *resto = buffer;
      tk = strtok_r(resto, "\n", &resto);
      printf("Received a datagram: %s \n", buffer);
      char msg[strlen(buffer)];
      strcpy(msg, buffer);
      mensagem = analisarComando(msg,isserver); //aqui já sei o que tenho que fazer
      printf("MENSAGEM:%s \t COMANDO:%d \tSEQUENCIA :%d \tVALOR :%d\n",buffer,mensagem.comando,mensagem.sequencia,mensagem.valor);
	  if(mensagem.comando==C_S_ERRO){
		  retorno[strlen(retorno)-2]='-';
		  retorno[strlen(retorno)-1]='1';
		  retorno[strlen(retorno)]='\0';
	  }else if (mensagem.comando<10){
		  retorno[strlen(retorno)-2]=' ';
          retorno[strlen(retorno)-1]=mensagem.comando+48;
		  retorno[strlen(retorno)]='\0';
      }else{
		  retorno[strlen(retorno)-2]=mensagem.comando/10+48;
		  retorno[strlen(retorno)-1]=mensagem.comando%10+48;
		  retorno[strlen(retorno)]='\0';
	  }
      n = sendto(sock, retorno, strlen(retorno)+1, 0, (struct sockaddr *)&from, fromlen);
	  printf("Enviando %s\n",retorno);
      if (n < 0)
      {
        error("Envio");
      }
      //bzero(buffer, BUFFER_SIZE);
    }
  }
  printf("Encerrando SERVIDOR\n");
}

//#################################################################//
//##################    Funções Auxiliares    #####################//
//#################################################################//
void error(const char *msg)
{
  perror(msg);
  exit(0);
}