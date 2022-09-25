#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h> // para non blocking sockets
#include "../headers/tempo.h"
#include "../headers/protocolo.h"
#include "../headers/kbhit.h"

#define DEBUG
#define BUFFER_SIZE 100
#define TAM 10
#define TIMEOUT 750 // milissegundos
#define WAIT 500
void error(const char *msg){
  perror(msg);
  exit(0);
}

void simula_comm(char buffer[]){
  char aux[TAM] = "\0";
  char STR_COMM[19] = "OpenValve#";

  snprintf(aux, TAM, "%d", random()%1000);
  strcat(STR_COMM,aux);
  strcat(STR_COMM,TK);
  bzero(aux,TAM);
  snprintf(aux, TAM, "%d", random()%100);
  strcat(STR_COMM,aux);
  strcat(STR_COMM,ENDMSG);
 // if(random()%11==0){
   //waitms(WAIT);
 // }
  printf("%s ",STR_COMM);
  strcpy(buffer,STR_COMM);
}

char teclado(){
  char r ='e';
  // bloco para captura de tecla
  if (kbhit()){ // LINUX não tem um kbhit() como o windows -> ver arquivo kbhit.h
    r = getchar();
  } 
  return r;
}

int main(int argc, char *argv[]){
  int sockfd, portno, n;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  struct timespec start;
  struct timespec atual;
  int long deltaT =0; 
  clockid_t clk_id;
  clk_id = CLOCK_MONOTONIC_RAW;
  char buffer[BUFFER_SIZE];
  char out = '\0'; // tecla em que irei guardar a saida
  int flagAguardo = 0;
  int perdido =0; 
  system("clear");
  srandom(time(NULL));   //Initialization, should only be called once.

  if (argc < 3)
  {
    fprintf(stderr, "usage %s hostname port\n", argv[0]);
    exit(0);
  }
  portno = atoi(argv[2]);
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  fcntl(sockfd, F_SETFL, O_NONBLOCK);
  if (sockfd < 0)
    error("ERROR opening socket");
  server = gethostbyname(argv[1]);
  if (server == NULL)
  {
    fprintf(stderr, "ERROR, no such host\n");
    exit(0);
  }
  bzero((char *)&serv_addr, sizeof(serv_addr)); //zera qualquer coisa
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr,
        (char *)&serv_addr.sin_addr.s_addr,
        server->h_length);
  serv_addr.sin_port = htons(portno);
  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR connecting");
  
  printf("Cliente inicializado, parabens\n");
  

  while (teclado() != '\n'){
    if (flagAguardo){ // estaria aguardando receber a resposta para continuar com uma nova escrita
      //printf("Continuo rodando\n");
      printf(".");
    }else{ // mandar uma  nova mensagem
      bzero(buffer, BUFFER_SIZE);
      //fgets(buffer, BUFFER_SIZE - 1, stdin);
      simula_comm(buffer);
      //out=buffer[0];
      n = write(sockfd, buffer, strlen(buffer));
      if (n < 0 && n != -1){ //
        error("ERROR writing to socket");
      }else{
        flagAguardo = 1;
      }
	  }
    bzero(buffer, BUFFER_SIZE);
   
    n = read(sockfd, buffer, BUFFER_SIZE-1);
    
    if (n < 0 && n != -1) // -1 quando não existe nada para ser lido
      error("ERROR reading from socket");
    else if (n != -1){
      printf("\tRECV: %s\n", buffer);
      flagAguardo = 0;
    }
  }
  n = write(sockfd, "\n", 1);
  close(sockfd);
  printf("Encerrando cliente\n\n");
  return 0;
}