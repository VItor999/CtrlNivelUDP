/// PROBLEMAS 
/// não faz sentido ler na main
/// na main atualizar a tela 
/// thread para a leitura 

#include "../headers/protocolo.h"
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
#define BUFFER_SIZE 1024
void error(char *msg);

pthread_mutex_t mutexCOM = PTHREAD_MUTEX_INITIALIZER;
char OUT = ' ';
TPMENSAGEM MENSAGEM;
int isserver = 0;

char teclado(){
  char r ='e';
  // bloco para captura de tecla
  if (kbhit())
  { // LINUX não tem um kbhit() como o windows -> ver arquivo kbhit.h
    r = getchar();
  } 
  return r;
}

void *threadComm(void *port){
  TPMENSAGEM mensagem;
  int sock, length, fromlen, n;
  struct sockaddr_in server;
  struct sockaddr_in from;
  char buffer[BUFFER_SIZE];
  char out = ' '; 
  char retorno [11] ="Comando   ";
  int flagNovaMsg = 0;
  char msg[strlen(buffer)];
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
  server.sin_port = *((int *)port); // converte o valor da porta para o formato necessário
  // Binding
  if (bind(sock, (struct sockaddr *)&server, length) < 0)
  {
    error("binding");
  }
  fromlen = sizeof(struct sockaddr_in);
  printf("Servidor iniciado em thread, parabens\n");
  // de onde vem, onde escrevo, tamanho, argumento padrao, estrutura e tamanho
   while (OUT != 27 && buffer[0] != '\n') // pressionando esq encerro o server
  {
    OUT = teclado(); 
    if (flagNovaMsg){ //pode dar ruim pois se tenho uma nova mesnsagem 
      if(pthread_mutex_trylock(&mutexCOM)==0){
        MENSAGEM = analisarComando(msg,isserver); //aqui já sei o que tenho que fazer
        printf("MENSAGEM:%s \t COMANDO:%d \tSEQUENCIA :%d \tVALOR :%d\n",buffer,MENSAGEM.comando,MENSAGEM.sequencia,MENSAGEM.valor);
        obterInfo(&mensagem,MENSAGEM);
        pthread_mutex_unlock(&mutexCOM);
        bzero(buffer, BUFFER_SIZE);
        flagNovaMsg=0;
      }
      n=0;// não faço nada enquanto tenho uma nova mensagem e não consegui me livrar dela 
    }
    else{
      // de onde vem, onde escrevo, tamanho, argumento padrao, estrutura e tamanho
      n = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&from, &fromlen);
    }
    
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
     
      strcpy(msg, buffer);
      if(pthread_mutex_trylock(&mutexCOM)==0 && buffer[0]!='\n'){
        MENSAGEM = analisarComando(msg,isserver); //aqui já sei o que tenho que fazer
        //printf("MENSAGEM:%s \t COMANDO:%d \tSEQUENCIA :%d \tVALOR :%d\n",buffer,MENSAGEM.comando,MENSAGEM.sequencia,MENSAGEM.valor);
        obterInfo(&mensagem,MENSAGEM);
        pthread_mutex_unlock(&mutexCOM);
        bzero(buffer, BUFFER_SIZE);
      }
      else if (buffer[0]=='\n'){
        OUT = 27;
      }
      else{
        flagNovaMsg  = 1;
      }
      
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
  //---- Variáveis UDP
  system("clear");
  pthread_t pthComm;       
  int r;
  //---- Variáveis 
  
  char out = '\0'; // tecla em que irei guardar a saida
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
  int porta = htons(atoi(argv[1]));
  r = pthread_create(&pthComm, NULL, threadComm, (void *)&porta);
  if (r)
  {
    fprintf(stderr, "Error - pthread_create() return code: %d\n",  r);
    exit(EXIT_FAILURE);
  }
  if (isserver){
    printf("Servidor iniciado para teste do servidor, parabens\n");
  }
   else{
    printf("Servidor iniciado para teste do cliente, parabens\n");
  }
  while (OUT != 27) // pressionando esq encerro o server
  {
    // bloco para captura de tecla
    //printf("Rodando MAIN ...\n");
    //OUT =  teclado();
    sleep(2);

    printf("MAIN:   COMANDO: %d   SEQUENCIA: %d   VALOR: %d\n",MENSAGEM.comando,MENSAGEM.sequencia,MENSAGEM.valor);
      // de onde vem, onde escrevo, tamanho, argumento padrao, estrutura e tamanho
  }
  pthread_join(pthComm, NULL);
  printf("Encerrando a main");
  exit(EXIT_SUCCESS);
  return 0;
}
