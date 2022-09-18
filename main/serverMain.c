#define DEBUG 1

#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <math.h>
#include "../headers/kbhit.h"
#include "../headers/protocol.h"
#include <time.h>
#define BUFFER_SIZE 1024

char INPUT[24] = "";
char OUTPUT[24] = "";
char OUT = '0';
int LASTCMD = 50;
int DATATOSEND = 0;
pthread_mutex_t mutexCOM = PTHREAD_MUTEX_INITIALIZER;

typedef struct S_ServerData
{
  /* data */
  in_port_t porta;
} ServerData;

void execComando(int r, char *memcpy);
void *threadServerUDP(void *arg);
void error(char *msg);

int main(int argc, char *argv[])
{
  // lembrando que: argc = total de argumentos da chamada
  //               *argv = argumentos recebidos (OBS: 0 = chamada do program, 1 - n o resto)
  // Argumentos recebibos sempre em array de char (string)
  // Só preciso passar a porta no servidor
  int iServer;
  int r;
  char memcpy[24];

  if (argc < 2) // chamada de programa + porta
  {
    fprintf(stderr, "ERROR, no port provided\n");
    exit(0);
  }
  ServerData serverData;
  serverData.porta = htons(atoi(argv[1])); // converte o valor da porta para o formato necessário
  pthread_t pthServidor;                   // threadServidor
  pthread_create(&pthServidor, NULL, threadServerUDP, (void *)&serverData);
  if (iServer)
  {
    fprintf(stderr, "Error - pthread_create() return code: %d\n", iServer);
    exit(EXIT_FAILURE);
  }
  while (OUT != 27 && INPUT[0] != '\n')
  {
    if (kbhit())
    { // LINUX não tem um kbhit() como o windows -> ver arquivo kbhit.h
      OUT = getchar();
    }
#ifdef DEBUG
    printf("Última Mensagem recebida %s\n", INPUT);
    sleep(2);
#endif
    r = LASTCMD;
    strcpy(memcpy, INPUT);
    printf("%s", INPUT);
    execComando(r, memcpy);
  }
  pthread_join(pthServidor, NULL);
  exit(EXIT_SUCCESS);
}

void execComando(int r, char *memcpy)
{
  char *tk = NULL;
  char *resto = memcpy;
  int numSeq, valor;
  char sSeq[4];
  char sValor[4];
  char aux[24] = "";
  if (r < 10) // comandos de saída AZUIS NO PDF
  {
    switch (r)
    {
      case C_O_CLOSE:
        // TODO fazer tratamento de erro para o número de sequencia e valor
        tk = strtok_r(resto, TK, &resto);            // elimina a primeira parte
        strcpy(sSeq, strtok_r(resto, TK, &resto));   // captura o número de sequencia
        strcpy(sValor, strtok_r(resto, TK, &resto)); // captura o valor
        strcat(aux, I_CLOSE);
        strcat(aux, TK);
        strcat(aux, sSeq);
        strcat(aux, ENDMSG);
        numSeq = atoi(sSeq);
        valor = atoi(sValor);
        strcpy(OUTPUT, aux);
        DATATOSEND = 1;
        break;
      case C_O_OPEN:
        // TODO fazer tratamento de erro para o número de sequencia e valor
        strcat(aux, I_OPEN);
        strcat(aux, TK);
        tk = strtok_r(resto, TK, &resto);          // elimina a primeira parte
        strcpy(sSeq, strtok_r(resto, TK, &resto)); // captura o número de sequencia
        printf("%s", sSeq);
        strcat(aux, sSeq);
        strcpy(sValor, strtok_r(resto, TK, &resto)); // captura o valor
        strcat(aux, ENDMSG);
        numSeq = atoi(sSeq);
        valor = atoi(sValor);

        strcpy(OUTPUT, aux);
#ifdef DEBUG
        printf("%s \tNumSeq %d, valor %d\n", tk, numSeq, valor);
        printf("Mensagem a ser enviada: %s\n", OUTPUT);
#endif
        DATATOSEND = 1;
        break;

        break;
      case C_O_SET:
        tk = strtok_r(resto, TK, &resto);          // elimina a primeira parte
        valor = atoi(strtok_r(resto, TK, &resto)); // captura o valor
#ifdef DEBUG
        printf("%s \tvalor %d\n", tk, valor);
#endif
        break;
      case C_O_GET:
        // DO STUFF
#ifdef DEBUG
        printf("%s \n", INPUT);
#endif
        break;
      case C_O_START:
        // DO STUFF
#ifdef DEBUG
        printf("%s \n", INPUT);
#endif
        break;
      case C_O_COM:
        // DO STUFF
#ifdef DEBUG
        printf("%s \n", INPUT);
#endif
        break;
      case C_I_ERRO:
        strcat(aux, I_ERRO);
        strcat(aux, ENDMSG);
        strcpy(OUTPUT, aux);
        DATATOSEND = 1;
#ifdef DEBUG
        printf("%s mensagem a ser enviada\n", OUTPUT);
#endif
        break;
      default:
        break;
    }
  }
  return;
}

void error(char *msg) // método para imprimir um erro, só passar uma mensagem
{
  perror(msg);
  exit(0);
}

void *threadServerUDP(void *arg)
{

  int sock, length, fromlen, n;
  struct sockaddr_in server;
  struct sockaddr_in from;
  char buffer[BUFFER_SIZE];
  char c = '\0'; // tecla em que irei guardar a saida
  char novaMsg = 0;
  sock = socket(AF_INET, SOCK_DGRAM, 0); // SOCK_STREAM -> TCP/IP tempo de break é o zero poderiamos tentar alterar para outro blg 1 ms??
  // Esse bagulho não bloquei, minima ideia de como opera dar uma pesquisada em como usar o rev
  fcntl(sock, F_SETFL, O_NONBLOCK);
  if (sock < 0)
  {
    error("Opening socket");
  }
  length = sizeof(server);
  bzero(&server, length); // limpando os valores do server
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = ((ServerData *)arg)->porta; // converte o valor da porta para o formato necessário
  // Binding
  if (bind(sock, (struct sockaddr *)&server, length) < 0)
  {
    error("binding");
  }
  fromlen = sizeof(struct sockaddr_in);
  printf("Servidor iniciado, parabens\n");
  // recvfrom recebe algo sendto envia
  while (OUT != 27 && buffer[0] != '\n') // pressionando esq encerro o server
  {
    // de onde vem, onde escrevo, tamanho, argumento padrao, estrutura e tamanho
    n = recvfrom(sock, buffer, BUFFER_SIZE-1, 0, (struct sockaddr *)&from, &fromlen);
    if (n < 0 && n != -1)
    {
      error("Receber");
    }
    pthread_mutex_lock(&mutexCOM);
    if (n > 0)
    {
#ifdef DEBUG // pois quando digito manualmente tenho que remover meu /n
      char *tk;
      char *resto = buffer;
      tk = strtok_r(resto, "\n", &resto);
      printf("Received a datagram: %s \n", buffer);
#endif
      // char mensagem[strlen(buffer)];

      strcpy(INPUT, buffer);
      // strcpy(mensagem, buffer);
      // int r = analisarComando(buffer); // aqui já sei o que tenho que fazer
      LASTCMD = analisarComando(INPUT, 1); // 1 pois é o servidor
      // = resp != -1 ? resp : LASTCMD; // se for invalida a mensagem mantem o último comando
#ifdef DEBUG
      printf("%d", LASTCMD);
#endif
      // passar isso por um switch ? escrever isso como retorno para a main?
      // colocar minha mensagem em algum lugar ?? area da main ?
      // Servidor é uma thread -> preciso passar uma serie de elementos para ele
      // preciso montar meu struct fora daqui e seguir a vida, passa ele para fazer o bindig na thread ???
      // Minha váriavel R será "global" e usarei mutex para lock e etc (uma vez) por ciclo eu atualizo ela ??
      // com base na variavel R sei o que fazer dar uma olhada em teste rapido.c
      // muitas ideias/ formas de fazer
      // precisamos definir melhor como vamos abordar o problema
      // sem isso eu não sei mt bem como prossseguir
      bzero(buffer, BUFFER_SIZE);
    }
    if (DATATOSEND)
    {
      // bzero(buffer, BUFFER_SIZE);
      //strcpy(buffer, OUTPUT);
      n = sendto(sock, OUTPUT, strlen(OUTPUT), 0, (struct sockaddr *)&from, fromlen);
      if (n < 0)
      {
        error("Envio");
      }
      else if (n > 0)
      {
        DATATOSEND = 0;
#ifdef DEBUG
        printf("Escreveu nova msg %s\n", OUTPUT);
#endif
      }
    }
    pthread_mutex_unlock(&mutexCOM);
  }
  //if (SAIR == 27)
  //{
    printf("Encerrando SERVIDOR");
  //}
  return NULL;
}