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
  int comando;

  //----- Teporização
  struct timespec clkPlanta;                     // Relógio do processo                  
  clockid_t clk_id = CLOCK_MONOTONIC_RAW;        // Para grarantir o avanço no relógio
  int attPlanta = 0 ;                            // Indica se deve atualizar a simulação                  
  clock_gettime(clk_id,&clkPlanta);              // Inicializa o relógio

  //---- Verificação dos parametros 
  if (argc < 2 && argc!=3){                     // chamada de programa + porta
    fprintf(stderr, "usage %s port ISSERVER\n",argv[0]);
    exit(0);
  }

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

  printf("Programa do servidor iniciado com SUCESSO.\n");
  clock_gettime(clk_id,&clkPlanta);

  //---- LOOP principal
  while (OUT != 27){
    // Variavel OUT é caputara dela thread gráfica
    attPlanta = deltaTempo(TPLANTA,clkPlanta);

    if (NOVAMENSAGEM){              
      comando = MENSAGEM.comando;
      if (comando == C_S_CLOSE || comando == C_S_OPEN ||   // Caso seja um comando útil
          comando == C_S_SET   || comando == C_S_START){
        pthread_mutex_lock(&mutexGRAPH);
        if(attPlanta){                                     // Deve Atualizar a planta?
          atualizarPlanta(MENSAGEM,&PLANTASIM,PARAMCICLO); // Atualiza
          attPlanta = 0;                                   // desliga a atualização da planta
          clock_gettime(clk_id,&clkPlanta);
        }
        else {
          atualizarPlanta(MENSAGEM,&PLANTASIM,PARAM);     // Atualize somente os parâmetros de simulação
        }
        if(MENSAGEM.comando == C_S_START){                // Inicia/Reinicia o porcesso.
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
      atualizarPlanta(vazia,&PLANTASIM,CICLO);            // Aqui não importa o conteudo da mensagem
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
  printf("\n===== Encerrando a Thread Principal  =====\n");
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
  //---- Comunicaçao
  int sock, length, fromlen, n = -1;
  struct sockaddr_in server;
  struct sockaddr_in from;
 
 
  //---- Auxiliares de comunicação
  TPMENSAGEM mensagem;
  char buffer[BUFFER_SIZE];                             // Bufffer Local
  char retorno [RETURN_SIZE]="\0";                      // Buffer com mensagem de retorno
   char msg[strlen(buffer)];                            // Buffer auxiliar de mensagem
  int flagNovaMsg = 0;                                  // Flag de nova mensagem
  int repetido = 0;                                     // Flag para indicar se uma mensagem/pacote é repetido

  //---- Temporização 
  struct timeval read_timeout;                  
  read_timeout.tv_sec = 0;
  read_timeout.tv_usec = uTIMEOUT;                      // Time out 
     
  //---- Configuração do SOCKET 
  sock = socket(AF_INET, SOCK_DGRAM,0);  
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);
  if (sock < 0){
    error("Opening socket");
  }

  //---- Inicializações auxiliares
  length = sizeof(server);
  fromlen = sizeof(struct sockaddr_in);
  msg[0]='\0';

  //---- Limpeza do socket 
  bzero(&server, length); // limpando os valores do server
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = *((int *)port); // converte o valor da porta para o formato necessário
 
  //---- Binding
  if (bind(sock, (struct sockaddr *)&server, length) < 0){
    error("binding");
  }
  
  //---- Notificação de sucesso
  printf("Servico de comunicação iniciado em thread\n");

  //---- LOOP de Comunicação
  while (OUT != 27 && msg[0]!='\n'){ // Encerrado pelo cliente ou por si próprio (Thread Gráfica)
    if(!flagNovaMsg){ // só altero n se leio 
      n = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&from, &fromlen);
    }
    if (n < 0 && n != -1){
      error("Receber");
    }else if (n > 0){                                   // captura de algo novo 
      if(!flagNovaMsg){
        char *tk;
        char *resto = buffer;
        tk = strtok_r(resto, "\n", &resto);
        #ifdef DEBUG
        if (buffer[0]!='\n'){
          printf("RECV %s", buffer);
        }
        #endif
        strcpy(msg, buffer);
        bzero(buffer, BUFFER_SIZE);
      }
      if(pthread_mutex_trylock(&mutexCOM)==0){ 
        mensagem = analisarComando(msg,ISSERVER);       // Verifica o conteudo da mensagem
        if(mensagem.comando == C_S_CLOSE || mensagem.comando == C_S_OPEN){
          repetido = verifica_tabela(mensagem);
        }else{
          repetido = 0;
        }
        if(!repetido){                                  // Verifica se não é repetida
          obterInfo(&MENSAGEM,mensagem);                // Escreve pro mundo
          NOVAMENSAGEM = 1;                             // Notifica o mundo
          pthread_mutex_unlock(&mutexCOM);              // Libera o mutex
          // Trava enquanto a informação disponível não for utilizada 
          while(MENSAGEM.comando != 0){}
          if(mensagem.comando == C_S_CLOSE              // Caso seja um comando que precise de registor
           || mensagem.comando == C_S_OPEN){
            atualiza_tabela(mensagem);
          }
          if (mensagem.comando == C_S_GET){
              mensagem.valor = PLANTASIM.nivel;        // Respostas 
          }
          else if(mensagem.comando == C_S_SET){
           mensagem.valor = PLANTASIM.max;             // Respostas
          }
        }else{                                         // Caso Mensagem repetida revisitar
          #ifdef DEBUG
          CONTRUIM ++;
          #endif
          pthread_mutex_unlock(&mutexCOM);
        }
        responde_cliente(mensagem, retorno);          // Responde ao cliente
        #ifdef TEMPESTADE                             // Perda de pacotes interna       
        if (random()%9 != 0) {
        #endif
          #ifdef TROVOADAS                            // Delay interno
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
  printf("\n==== Encerrando Thread de Comunicação ====\n");
}

/**
*@brief Thread que executa o parte gráfica do programa. É atualizada conforme a definição de TGRAPH
*     
*@param args 
*@return void* 
**/
void* threadGraph(void* args)
{
    //---- Auxiliares 
    int taux=0;
    int attGraphTime = 0;
    int rodar=0;

    //---- Temporização 
    static long int tempo =0;
    struct timespec clkGraph;
    clockid_t clk_id = CLOCK_MONOTONIC_RAW;

    //---- Inicialização
    clock_gettime(clk_id,&clkGraph);
    #ifdef GRAPH
    Tdataholder *data;
    data = datainit(640,480,150,120,(double)LVINIC,(double)0,(double)0);
    datadraw(data,0,LVINIC,50,outAngle(0));
    #endif
    //---- Notificação de sucesso
    printf("Thread Gráfica Iniciada\n");
    
      //---- LOOP Gráfico principal
    while(OUT != 27){
      if(INICIARGRAPH==1 || rodar==-1){  
        if(pthread_mutex_trylock(&mutexGRAPH)==0){         // Caso não tenha iniciado ou não deva executar
          INICIARGRAPH = 0;
        if(rodar ==0)  clock_gettime(clk_id,&clkGraph);
          rodar =1;                                        // Habilita a execução
          #ifdef GRAPH
          Restart(640,480,150,120,(double)LVINIC,(double)0,(double)0,data);
          #endif
          pthread_mutex_unlock(&mutexGRAPH);
        } 
      }
      if(rodar==1){                                       // Se deve executar
            attGraphTime = deltaTempo(TGRAPH,clkGraph);   // Atualiza status do contador
        if(attGraphTime){
	          tempo+= TGRAPH;                               // Aumenta o passo
            taux = tempo%150000;                          // Verifica se chegou a fim do gráfico (150s)
            pthread_mutex_lock(&mutexGRAPH);
            #ifdef GRAPH
            datadraw(data,(double)taux/1000.0,(double)PLANTASIM.nivel,(double)PLANTASIM.angIN,(double)PLANTASIM.angOUT);
            #endif
            #ifndef GRAPH
            printf("PLANTA:\tT-%6ld\tN-%3d\tV-%3.3f\tP-%3.3f\n",tempo,PLANTASIM.nivel,PLANTASIM.angIN,PLANTASIM.angOUT);
            #endif
            pthread_mutex_unlock(&mutexGRAPH);
            
            if ((taux)==0){                              // Se chegou ao fim do gráfico
              rodar = -1;                                // Forçar a limpeza do gráfico
            }
            tryExit();                                   // A cada 50 ms verifica se o User deseja sair 
            clock_gettime(clk_id,&clkGraph);             // Atualiza o relógio
        }
      }
      else{
        tryExit();                                       // Sempre Verifica se o usuário deseja sair
      }
     
    }
    printf("\n======== Encerrando Thread Gráfica =======\n");
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
*@param msg Estrutura com os dados da resposta 
*@param ans Bufffer onde deve ser escrita a string que é a mensagem propriamente dita 
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
*@param msg Mensagem recebida 
*@return int  retorn 1 se encontrou 
**/
int verifica_tabela(TPMENSAGEM msg){
  int i, flag_retorno = 0; //inicializa flag de retorno para o padrao sem "erro"
  //Verifica se alguma linha da tabela possui a mesma correspondencia
  if(TABREINIC == 0){ 
    for(i=LINHAATUAL;(i>LINHAATUAL-TOL && i>=0 && flag_retorno!=1);i--){
      if(msg.comando == TABELA[i][0] && msg.valor == TABELA[i][2] && LINHAATUAL!=0 && MENSAGEM.sequencia == TABELA[i][1]){
        flag_retorno = 1;           //ativa flag de retorno para "erro"
      }
    }
  }
  else{
     // 31 .... 39 -> primeiro for 
     for(i=LIN-LINHAATUAL-TOL;(i<LIN && flag_retorno!=1);i++){
      if(msg.comando == TABELA[i][0] && msg.valor == TABELA[i][2] && LINHAATUAL!=0 && MENSAGEM.sequencia == TABELA[i][1]){
        //printf("\tREP!!!");
        flag_retorno = 1;           //ativa flag de retorno para "erro"
      }
    }
    for(i =0; (i<LINHAATUAL && flag_retorno!=1);i++){
       if(msg.comando == TABELA[i][0] && msg.valor == TABELA[i][2] && LINHAATUAL!=0 && MENSAGEM.sequencia == TABELA[i][1]){
        flag_retorno = 1;           //ativa flag de retorno para "erro"
      }
    }
  }
  return flag_retorno;
}

#ifdef DEBUG
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
#endif
