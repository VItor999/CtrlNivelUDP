// TODO:
// Pensar na rotina de simulação
// Pensar na forma de armazenamento de mensagens 
// Pensar no cliente e em como fazer-lo operar

//===================== Bibliotecas utilizadas =====================//

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
#include "../headers/protocolo.h"
#include "../headers/tempo.h"

//====================== Definições efetuadas ======================//

#define DEBUG 1 
#define LIN 40  
#define COL 3
#define TOL 10 //tolerancia de linhas da tabela para conferencia de comando repetido

#define BUFFER_SIZE 100


//======================= Variáveis Globais  ======================//

pthread_mutex_t mutexCOM = PTHREAD_MUTEX_INITIALIZER;
char OUT = ' ';
TPMENSAGEM MENSAGEM;
int isserver = 0;
int TABELA[LIN][COL];
int LINHAATUAL =0;    		// linha atual da tabela
int NOVAMENSAGEM =0;
//===================== Cabeçalhos de Funções =====================//

char teclado();
void *threadComm(void *port);
void error(char *msg);
void atualiza_tabela(TPMENSAGEM msg);
int verifica_tabela(TPMENSAGEM msg);
void simulador();
void imprime_tabela();
void responde_cliente(TPMENSAGEM msg, char ans[]);

//#################################################################//
//#########################    MAIN    ############################//
//#################################################################//

int main(int argc, char *argv[]){
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

  if (argc < 2 && argc!=3){  // chamada de programa + porta
    fprintf(stderr, "usage %s port isserver\n",argv[0]);
    exit(0);
  }else if (argc==2) { // se passar direto simulo servidor
    isserver = 1;
  }else{
    isserver = atoi(argv[2]);
  }
  int porta = htons(atoi(argv[1]));
  r = pthread_create(&pthComm, NULL, threadComm, (void *)&porta);
  if (r){
    fprintf(stderr, "Error - pthread_create() return code: %d\n",  r);
    exit(EXIT_FAILURE);
  }
  if (isserver){
    printf("Servidor iniciado para teste do servidor, parabens\n");
  }else{
    printf("Servidor iniciado para teste do cliente, parabens\n");
  }


  while (OUT != 27){ // pressionando esq encerro o server
    // bloco para captura de tecla
    //printf("Rodando MAIN ...\n");
    //OUT =  teclado();
    //waitms(999);
    if (NOVAMENSAGEM && pthread_mutex_trylock(&mutexCOM)==0){
      printf("\tCOM %d\tSEQ %d\tVAL %d",MENSAGEM.comando,MENSAGEM.sequencia,MENSAGEM.valor);
      simulador();
      NOVAMENSAGEM = 0;
      pthread_mutex_unlock(&mutexCOM);
    }
  }
  pthread_join(pthComm, NULL);
  printf("Encerrando main\n\n");
  return 0;
}


//#################################################################//
//##################    Funções Auxiliares    #####################//
//#################################################################//

void *threadComm(void *port){
  #define RETURN_SIZE 10
  TPMENSAGEM mensagem;
  int sock, length, fromlen, n = -1;
  struct sockaddr_in server;
  struct sockaddr_in from;
  char buffer[BUFFER_SIZE];
  char out = ' '; 
  char retorno [RETURN_SIZE]="\0";
  int flagNovaMsg = 0;
  char msg[strlen(buffer)];
  sock = socket(AF_INET, SOCK_DGRAM, 0); // SOCK_STREAM -> TCP/IP tempo de break é o zero poderiamos tentar alterar para outro blg 1 ms??
  // Esse bagulho não bloquei, minima ideia de como opera dar uma pesquisada em como usar o rev
  fcntl(sock, F_SETFL, O_NONBLOCK);
  if (sock < 0){
    error("Opening socket");
  }
  length = sizeof(server);
  bzero(&server, length); // limpando os valores do server
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = *((int *)port); // converte o valor da porta para o formato necessário
  // Binding
  if (bind(sock, (struct sockaddr *)&server, length) < 0){
    error("binding");
  }
  fromlen = sizeof(struct sockaddr_in);
  printf("Servico de comunicação iniciado em thread\n");
  
  while (OUT != 27){ // pressionando esq encerro o server
    OUT = teclado(); 
    if(!flagNovaMsg){
      n = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&from, &fromlen);
    }
    if (n < 0 && n != -1){
      error("Receber");
    
    }else if (n > 0){// captura de algo novo 
      char *tk;
      char *resto = buffer;
      tk = strtok_r(resto, "\n", &resto);
      #ifdef DEBUG
      if (buffer[0]!='\n')printf("RECV %s", buffer);
      #endif
      
      strcpy(msg, buffer);
      bzero(buffer, BUFFER_SIZE);
      
      if (msg[0]=='\n'){
        OUT = 27;
      
      }else if(pthread_mutex_trylock(&mutexCOM)==0 || flagNovaMsg){ //peguei mutex
        mensagem = analisarComando(msg,isserver);
        if(verifica_tabela(mensagem)==0){
          obterInfo(&MENSAGEM,mensagem);  //Escreve pro mundo
          NOVAMENSAGEM = 1;
          pthread_mutex_unlock(&mutexCOM);

          while(MENSAGEM.comando != 0){ // fica travado esperando o simulador dizer que usou a info
            //printf("AGUARDANDO SIMULADOR\n");
          }
          atualiza_tabela(mensagem);
          //imprime_tabela();
          responde_cliente(mensagem, retorno);
          /*if(mensagem.comando==C_S_ERRO){
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
          }*/
        }else{ //caso msg repetida
          pthread_mutex_unlock(&mutexCOM);
          //bzero(buffer, BUFFER_SIZE);
          /*retorno[strlen(retorno)-2]='-';
          retorno[strlen(retorno)-1]='1';
          retorno[strlen(retorno)]='\0';*/
          mensagem.comando=C_S_ERRO;
          responde_cliente(mensagem, retorno);
        }

        n = sendto(sock, retorno, strlen(retorno)+1, 0, (struct sockaddr *)&from, fromlen);
        if (n < 0){
          error("Envio");
        }else{
          printf("\tSEND %s\n",retorno);
        }
        bzero(retorno,RETURN_SIZE);
        flagNovaMsg = 0;

      }else{ //nao consegui mutex e preciso eventualmente me livrar da msg
        flagNovaMsg = 1;
      }
    }
  }
  printf("\nEncerrando thread\n");
}

void responde_cliente(TPMENSAGEM msg, char ans[]){
  #define TAM 10
  char aux[TAM]="\0";
  switch (msg.comando){
  case C_S_OPEN:
    strcat(ans,S_OPEN);
    strcat(ans,TK);
    snprintf(aux, TAM, "%d", msg.sequencia);
    strcat(ans,aux);
    break;
  case C_S_CLOSE:
    strcat(ans,S_CLOSE);
    strcat(ans,TK);
    snprintf(aux, TAM, "%d", msg.sequencia);
    strcat(ans,aux);
    break;
  case C_S_GET:
    strcat(ans,S_GETLV);
    strcat(ans,TK);
    snprintf(aux, TAM, "%d", msg.valor);
    strcat(ans,aux);
    break;
  case C_S_SET:
    strcat(ans,S_SETMAX);
    strcat(ans,TK);
    snprintf(aux, TAM, "%d", msg.valor);
    strcat(ans,aux);
    break;
  case C_S_COM:
    strcat(ans,S_COMTEST);
    strcat(ans,TK);
    strcat(ans,OK);
    break;
  case C_S_START:
    strcat(ans,S_START);
    strcat(ans,TK);
    strcat(ans,OK);
    break;
  
  default:
    strcat(ans,S_ERRO);
    break;
  }
  strcat(ans,ENDMSG);
}

void atualiza_tabela(TPMENSAGEM msg){
  TABELA[LINHAATUAL][0]=msg.comando;
  TABELA[LINHAATUAL][1]=msg.sequencia;
  TABELA[LINHAATUAL][2]=msg.valor;
  LINHAATUAL++;
}

int verifica_tabela(TPMENSAGEM msg){
  int i, flag_retorno = 0; //inicializa flag de retorno para o padrao sem "erro"
  //Verifica se alguma linha da tabela possui a mesma correspondencia
  //int cont=0;
  for(i=LINHAATUAL;(i>LINHAATUAL-TOL && i>=0 && flag_retorno!=1);i--){
    if(msg.comando == TABELA[i][0] && msg.valor == TABELA[i][2] && LINHAATUAL!=0){//&& MENSAGEM.sequencia == TABELA[i][j])
      printf("\tMENSAGEM REPETIDA!!!");
      flag_retorno = 1; //ativa flag de retorno para "erro"
    }
    //cont++;
  }
  //printf("\tcont: %d\n",cont);
  return flag_retorno;
}

void simulador(){//simula simulador kkj
  MENSAGEM.comando=0;
}

void imprime_tabela(){
  int i;
  for(i=0; i<LINHAATUAL;i++){
    printf("(%d)\t%d, %d, %d\n",i,TABELA[i][0],TABELA[i][1],TABELA[i][2]);
  }
}

void error(char *msg){ // método para imprimir um erro, só passar uma mensagem
  perror(msg);
  exit(0); // se colocar exit(1) ele não sai do programa
}

char teclado(){
  char r ='e';
  // bloco para captura de tecla
  if (kbhit()){ // LINUX não tem um kbhit() como o windows -> ver arquivo kbhit.h
    r = getchar();
  } 
  return r;
}