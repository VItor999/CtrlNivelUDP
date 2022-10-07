/**
*@file serverv1.c
*@author Lucas Esteves e Vitor Carvalho 
*@brief Código fonte do servidor do trabalho 4 
*@version FINAL 
*@date 2022-10-01
*
**/

//===================== Bibliotecas utilizadas =====================//
#define GRAPH 1                               //comentar para desabilitar gráficos 

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
#ifdef GRAPH
#include "../headers/graph.h"
#endif
#include "../headers/planta.h"

//====================== Definições efetuadas ======================//

//#define DEBUG 1             // Habilita alguns prints de Debug 
//#define TEMPESTADE 1        // Habilita perda de pacotes
//#define TROVOADAS 1         // Habilita delay entre pacotes  
//#define DELAY 100              // Delay inserido pela simulação de delay

#define LIN 5000               /* Número de linhas da tabela de Pacotes Perdidos */ 
                              // TODO: fazer uma tabela dinamica
#define COL 3                 /* Número de colunas da tabela de Pacortes Perdidos */
#define TOL 10                /* Tolerância de linhas da tabela para conferência de comando repetido */
#define BUFFER_SIZE 100       /* Tamanho do buffer de comunicação*/
#define RETURN_SIZE 15        /* Tamanho da variável de retorno */
#define TGRAPH  50            /* Taxa de atualização do gráfico em ms*/       
#define ISSERVER 1            /* Define como um servidor */

 

//======================= Variáveis Globais  ======================//

pthread_mutex_t mutexGRAPH = PTHREAD_MUTEX_INITIALIZER;       /* MUTEX para a Thread Gráfica*/
pthread_mutex_t mutexCOM = PTHREAD_MUTEX_INITIALIZER;         /* MUTEX para a Thread de Comunicação*/
//pthread_mutex_t mutexSIM = PTHREAD_MUTEX_INITIALIZER;
char OUT = '\0';                                              /* Variável para encerramento do servidor*/
TPMENSAGEM MENSAGEM;                                          /* Variável Global que contêm as informações de uma mensagem*/
TPPLANTA PLANTASIM;                                           /* Variável Global que contêm as informações do processo simulado*/
int TABELA[LIN][COL]={0};                                     /* Tabela que contêm lista de pacotes perdidos*/
int NOVAMENSAGEM =0;                                          /* Flag global para indicar uma nova mensagem*/
int LINHAATUAL = 0;    		                                    /* Variável global que indica a linha atual da tabela */
int TABREINIC = 0;                                            /* Flag global para indicar o reinicio do preenchimento da tabela */
int INICIARGRAPH = -1;                                         /* Flag global para indicar que o gráfico deve ser reiniciado     */
int ATTGRAPH = 0;                                             /* Flag global para indicar que o gráfico deve ser atualizado     */
int PLANTAATIVA =0;                                           /* Flag global para indicar que a planta está ativa               */
#ifdef DEBUG
int CONTRUIM =0 ;                                             // Indica o número de pacotes perdios ou repetidos recebidos
#endif

//===================== Cabeçalhos de Funções =====================//

void *threadGraph(void* args);                                
void *threadComm(void *port);
void atualiza_tabela(TPMENSAGEM msg);
int  verifica_tabela(TPMENSAGEM msg);
void responde_cliente(TPMENSAGEM msg, char ans[]);
void imprime_tabela();
void tryExit();

//#################################################################//
//#########################    MAIN    ############################//
//#################################################################//

int main(int argc, char *argv[]){
  // lembrando que: argc = total de argumentos da chamada
  //               *argv = argumentos recebidos (OBS: 0 = chamada do program, 1 - n o resto)
  // Argumentos recebibos sempre em array de char (string)
  // Só preciso passar a porta no servidor
  //---- Inicializações
  system("clear");
  #ifdef DEBUG 
  srandom(time(NULL));
  #endif
  //---- UDP
  int porta;

  //---- Threads 
  pthread_t pthComm;
  pthread_t pthGraph;        
  
  //---- Variáveis Auxiliares 
  int r1,r2;
  TPMENSAGEM vazia;
  vazia.comando = VAZIO;
  vazia.sequencia = VAZIO;
  vazia.valor = VAZIO;

  //----- Teporização
  struct timespec clkPlanta;
  struct timespec clkGraph;
  clockid_t clk_id = CLOCK_MONOTONIC_RAW;
  int attPlanta = 0 ;
  int attGraphTime = 0;
  int comando;

  //---- Verificação dos parametros 
  if (argc < 2 && argc!=3){  // chamada de programa + porta
    fprintf(stderr, "usage %s port ISSERVER\n",argv[0]);
    exit(0);
  }
  //else if (argc==2) { // se passar direto simulo servidor
  //  ISSERVER = 1;
  //}else{
  //  ISSERVER = atoi(argv[2]);
  //}

  //---- Captura de dado relevante para o UDP
  porta = htons(atoi(argv[1]));

  //---- Cria e  Testa a thread
  r1 = pthread_create(&pthComm, NULL, threadComm, (void *)&porta);
  if (r1){
    fprintf(stderr, "Error - pthread_create() COMM return code: %d\n",  r1);
    exit(EXIT_FAILURE);
  }
  r2 =pthread_create( &pthGraph, NULL, threadGraph, NULL);
  if(r2)
  {
    fprintf(stderr, "Error - pthread_create() GRAPH return code: %d\n",  r2);
    exit(EXIT_FAILURE);
  }

  // Notificação de sucesso
  //if (ISSERVER){
    printf("Programa do servidor iniciado com SUCESSO.\n");
  //}else{
  //  printf("Servidor iniciado para teste do cliente, parabens\n");
  //}
 
  //---- Início nos timers
  clock_gettime(clk_id,&clkPlanta);
 

  //---- LOOP principal
  while (OUT != 27){ // pressionando esq encerro o server PRECISO DAR JEITO DE FAZER A TEMPORIZACAO
    // Variavel OUT é caputara dela thread que está sempre operando -> COMM
    attPlanta = deltaTempo(TPLANTA,clkPlanta);

    if (NOVAMENSAGEM){// && pthread_mutex_trylock(&mutexCOM)==0){
      comando = MENSAGEM.comando;
      //printf("\tCOM %d\tSEQ %d\tVAL %d",comando,MENSAGEM.sequencia,MENSAGEM.valor);
      if (comando == C_S_CLOSE || comando == C_S_OPEN ||
          comando == C_S_SET   || comando == C_S_START){
        pthread_mutex_lock(&mutexGRAPH);
        if(attPlanta){
          atualizarPlanta(MENSAGEM,&PLANTASIM,PARAMCICLO);
          attPlanta = 0; // desliga a atualização da planta
          clock_gettime(clk_id,&clkPlanta);
        }
        else {
          atualizarPlanta(MENSAGEM,&PLANTASIM,PARAM);
        }
        if(MENSAGEM.comando == C_S_START){
          INICIARGRAPH = 1;
          PLANTAATIVA = 1;
          printf("(Re)Iniciando Processo\n");
        }
        pthread_mutex_unlock(&mutexGRAPH);
      }
      // Atribui zero ao comando, indicando que já trabalhou-se com a mensagem
      MENSAGEM.comando =0;
      pthread_mutex_lock(&mutexCOM);
      NOVAMENSAGEM = 0;
      pthread_mutex_unlock(&mutexCOM);
    }
    if(attPlanta && PLANTAATIVA){
      pthread_mutex_lock(&mutexGRAPH);
      atualizarPlanta(vazia,&PLANTASIM,CICLO); // aqui não importa o conteudo da msg
      attPlanta = 0;
      clock_gettime(clk_id,&clkPlanta);
      pthread_mutex_unlock(&mutexGRAPH);
    }
    
  }

  //---- Encerrando
  pthread_join(pthComm, NULL);
  pthread_join(pthGraph, NULL);
  //imprime_tabela();
  #ifdef DEBUG
  printf("Pacotes Perdidos + repetidos:\t%d\n", CONTRUIM );
  #endif
  printf("Encerrando main\n\n");
  return 0;
}


//#################################################################//
//##################    Funções Auxiliares    #####################//
//#################################################################//

/**
*@brief Thread que efetua a comunicação com o cliente, captura, interpreta  e envia uma mensagem
*
*@param port // porta onde está localizado o servidor
*@return void* 
**/
void *threadComm(void *port){
  TPMENSAGEM mensagem;
  int sock, length, fromlen, n = -1;
  struct sockaddr_in server;
  struct sockaddr_in from;
  char buffer[BUFFER_SIZE];
  char retorno [RETURN_SIZE]="\0";
  int flagNovaMsg = 0;
  char msg[strlen(buffer)];
  int repetido = 0;
  int nivel;
  sock = socket(AF_INET, SOCK_DGRAM,0); 
  struct timeval read_timeout;
  read_timeout.tv_sec = 0;
  read_timeout.tv_usec = uTIMEOUT;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);
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
  msg[0]='\0';
  printf("Servico de comunicação iniciado em thread\n");

  while (OUT != 27 && msg[0]!='\n'){ // pressionando esq encerro o server
    
    if(!flagNovaMsg){ // só altero n se leio 
      n = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&from, &fromlen);
      //printf("sai");
    }
    if (n < 0 && n != -1){
      error("Receber");
    }else if (n > 0){// captura de algo novo 
      if(!flagNovaMsg){
        char *tk;
        char *resto = buffer;
        tk = strtok_r(resto, "\n", &resto);
        #ifdef DEBUG
        if (buffer[0]!='\n'){
          //printf("RECV %s", buffer);
        }
        #endif
        strcpy(msg, buffer);
        bzero(buffer, BUFFER_SIZE);
        //if (msg[0]=='\n'){ // desligar servidor 
        //  OUT = 27;
        //}
      }
      if(pthread_mutex_trylock(&mutexCOM)==0){ //peguei mutex
        mensagem = analisarComando(msg,ISSERVER);
        if(mensagem.comando == C_S_CLOSE || mensagem.comando == C_S_OPEN){
          //printf("VERIFICA TABELA!!!\n");
          repetido = verifica_tabela(mensagem);
        }else{
          repetido = 0;
        }
        if(!repetido){ // verifica se não é repetida
          obterInfo(&MENSAGEM,mensagem);  //Escreve pro mundo
          NOVAMENSAGEM = 1;  //notifica o mundo
          pthread_mutex_unlock(&mutexCOM); // libera o mutex
          // -Trava enquanto a informação disponível não for utilizada 
          while(MENSAGEM.comando != 0){ // fica travado esperando o simulador dizer que usou a info
            //printf("AGUARDANDO SIMULADOR\n");
          }
          if(mensagem.comando == C_S_CLOSE || mensagem.comando == C_S_OPEN){
            atualiza_tabela(mensagem);
          }
          //pthread_mutex_lock(&mutexSIM); // garante que não irá atrapalhar a simulação
          if (mensagem.comando == C_S_GET) mensagem.valor = PLANTASIM.nivel; //respostas 
          else if(mensagem.comando == C_S_SET) mensagem.valor = PLANTASIM.max; // respostas
          //pthread_mutex_unlock(&mutexSIM);
          //responde_cliente(mensagem, retorno);
        }else{ //caso msg repetida revisitar 
          #ifdef DEBUG
          CONTRUIM ++;
          #endif
          pthread_mutex_unlock(&mutexCOM);
          //responde_cliente(mensagem, retorno);
        }
        responde_cliente(mensagem, retorno);
        //responder o cliente 
        #ifdef TEMPESTADE
        if (random()%9 != 0) {
        #endif
          #ifdef TROVOADAS
          if(random()%7 == 0){  
            waitms(DELAY);
            printf("\nDELAY\n");
          }else{
            //printf("\t");
          }
          #endif
          // altero n com um envio 
          n = sendto(sock, retorno, strlen(retorno)+1, 0, (struct sockaddr *)&from, fromlen);
          if (n < 0){
            error("Envio");
          }else{
            //printf("\tSEND %s\n",retorno);
          }
        #ifdef TEMPESTADE
        }
        else{
          #ifdef DEBUG
          CONTRUIM ++;
          #endif
          printf("\nPERDEU\n");

        }
        #endif
        bzero(retorno,RETURN_SIZE);
        flagNovaMsg = 0;
      }else{ //nao consegui mutex e preciso eventualmente me livrar da msg
        flagNovaMsg = 1;
      }
    }
  }
  OUT = 27;
  printf("\nEncerrando thread\n");
}

/**
*@brief Thread que executa o parte gráfica do programa. É atualizada conforme a definição de TGRAPH
*     
*@param args 
*@return void* 
**/
void* threadGraph(void* args)
{
    int exit =0, ctrl=0, comando=0;
    int hasStarted = 0;
    int atualizarPlot = 0;
    static int tempo =0;
    #ifdef GRAPH
    Tdataholder *data;
    data = datainit(640,480,150,120,(double)LVINIC,(double)0,(double)0);
    datadraw(data,0,LVINIC,50,outAngle(0));
    #endif
    printf("Thread Gráfica Iniciada\n");
    tempo =0;
    int taux=0;
    struct timespec clkGraph;
    clockid_t clk_id = CLOCK_MONOTONIC_RAW;
    int attGraphTime = 0;
    int rodar=0;
    clock_gettime(clk_id,&clkGraph);
    while(OUT != 27){
      if(INICIARGRAPH==1 || rodar==-1){  // se não comecei -> o importante é ver se tenho que começar
        if(pthread_mutex_trylock(&mutexGRAPH)==0){
          INICIARGRAPH = 0;
        // botar o limpar grafico e inicio aqui
        if(rodar ==0)  clock_gettime(clk_id,&clkGraph);
          rodar =1;
          Restart(640,480,150,120,(double)LVINIC,(double)0,(double)0,data);
          pthread_mutex_unlock(&mutexGRAPH);
        } 
      }
      if(rodar==1){
            attGraphTime = deltaTempo(TGRAPH,clkGraph);
        if(attGraphTime){
	          tempo+= TGRAPH;
            taux = tempo%150000;
            pthread_mutex_lock(&mutexGRAPH);
            #ifdef GRAPH
            datadraw(data,(double)taux/1000.0,(double)PLANTASIM.nivel,(double)PLANTASIM.angIN,(double)PLANTASIM.angOUT);
            #endif
            #ifndef GRAPH
            printf("PLANTA:\tT-%6ld\tN-%3d\tV-%3.3f\tP-%3.3f\n",tempo,PLANTASIM.nivel,PLANTASIM.angIN,PLANTASIM.angOUT);
            #endif
            pthread_mutex_unlock(&mutexGRAPH);
            
            if ((taux)==0){
              rodar = -1;
              taux =0;
            }
          //  printf("%ld\n",tempo);
            tryExit();
            clock_gettime(clk_id,&clkGraph);
        }
      }
      else{
        tryExit();
      }
     
    }
    printf("FIM GRAFICO\n");
}

void tryExit(){
    //OUT = teclado(); 
    #ifdef GRAPH
    quitevent(&OUT);
    #endif
}

/**
*@brief Função que gera uma resposta para o servidor ...
*
*@param msg 
*@param ans 
**/
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

/**
*@brief Função que atualiza a tabela 
* 
*@param msg Mensagem para atualização
**/
void atualiza_tabela(TPMENSAGEM msg){
  TABELA[LINHAATUAL][0]=msg.comando;
  TABELA[LINHAATUAL][1]=msg.sequencia;
  TABELA[LINHAATUAL][2]=msg.valor;
  if (LINHAATUAL == LIN - 1){
    LINHAATUAL = 0; // reinicia ciclo
    TABREINIC = 1;
  }
  else if (TABREINIC == 1 && LINHAATUAL - TOL ==0){
    TABREINIC = 0;
    LINHAATUAL++; 
  }
  else{
    LINHAATUAL++; 
  }
}

/**
*@brief Verifica se o comando recebido já não está cadastrado na tabela
*
*@param msg 
*@return int 
**/
int verifica_tabela(TPMENSAGEM msg){
  int i, flag_retorno = 0; //inicializa flag de retorno para o padrao sem "erro"
  //Verifica se alguma linha da tabela possui a mesma correspondencia
  //int cont=0;
  if(TABREINIC == 0){ 
    for(i=LINHAATUAL;(i>LINHAATUAL-TOL && i>=0 && flag_retorno!=1);i--){
      if(msg.comando == TABELA[i][0] && msg.valor == TABELA[i][2] && LINHAATUAL!=0 && MENSAGEM.sequencia == TABELA[i][1]){//
        //printf("\tREP!!!");
        flag_retorno = 1; //ativa flag de retorno para "erro"
      }
      //cont++;
    }
  }
  else{
     // 31 .... 39 -> primeiro for 
     // zero flag 
     for(i=LIN-LINHAATUAL-TOL;(i<LIN && flag_retorno!=1);i++){
      if(msg.comando == TABELA[i][0] && msg.valor == TABELA[i][2] && LINHAATUAL!=0 && MENSAGEM.sequencia == TABELA[i][1]){//
        //printf("\tREP!!!");
        flag_retorno = 1; //ativa flag de retorno para "erro"
      }
      //cont++;
    }
    for(i =0; (i<LINHAATUAL && flag_retorno!=1);i++){
       if(msg.comando == TABELA[i][0] && msg.valor == TABELA[i][2] && LINHAATUAL!=0 && MENSAGEM.sequencia == TABELA[i][1]){//
        //printf("\tREP!!!");
        flag_retorno = 1; //ativa flag de retorno para "erro"
      }
    }
  }
  // 0 1 2 3 4 
  //printf("\tcont: %d\n",cont);
  return flag_retorno;
}


/**
*@brief Imprime a tabela, somente para debug.
*
**/
void imprime_tabela(){
  int i;
  for(i=0; i<LIN;i++){
    printf("(%d)\t%d, %d, %d\n",i,TABELA[i][0],TABELA[i][1],TABELA[i][2]);
  }
}