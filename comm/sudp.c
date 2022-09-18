/* Creates a datagram server. The port number is passed as an argument. This server runs forever */
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include "kbhit.h"
#define BUFFER_SIZE 1024

void error(char *msg) // método para imprimir um erro, só passar uma mensagem
{
  perror(msg);
  exit(0); // se colocar exit(1) ele não sai do programa
}

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
  if (argc < 2)  // chamada de programa + porta
  {
    fprintf(stderr, "ERROR, no port provided\n");
    exit(0);
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
  printf("Servidor iniciado, parabens\n");
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
      printf("Received a datagram: %s \n", buffer);
      n = sendto(sock, buffer, strlen(buffer)+1, 0, (struct sockaddr *)&from, fromlen);
      if (n < 0)
      {
        error("Envio");
      }
    }
  }
  printf("Encerrando SERVIDOR\n");
}