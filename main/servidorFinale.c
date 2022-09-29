// TODO:
// AJUSTAR ONDE ZERA A MENSAGEM PRA MATAR A FUNÇÂO SIMULADOR 

//===================== Bibliotecas utilizadas =====================//
//#define GRAPH 1

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

#define DEBUG 1 
#define TEMPESTADE 1
#define TROVOADAS 1
#define LIN 100  
#define COL 3
#define TOL 10 //tolerancia de linhas da tabela para conferencia de comando repetido
#define BUFFER_SIZE 100
#define RETURN_SIZE 10
#define DELAY 700
#define TGRAPH  200 //ms


//======================= Variáveis Globais  ======================//
pthread_mutex_t mutexGRAPH = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexCOM = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t mutexSIM = PTHREAD_MUTEX_INITIALIZER;
char OUT = '\0';
TPMENSAGEM MENSAGEM;
TPPLANTA PLANTASIM;
int NOVAMENSAGEM =0;   /* Flag para indicar nova mensagem*/
int TABELA[LIN][COL]={0};
int LINHAATUAL = 0;    		// linha atual da tabela
int TABREINIC = 0;      /* Flag para indicar o reinicio do preenchimento da tabela */
int INICIARGRAPH = 0;   /* Flag para indicar que o gráfico deve ser reiniciado     */
int ATTGRAPH = 0;       /* Flag para indicar que o gráfico deve ser atualizado     */
int PLANTAATIVA =0;      /* Flag para indicar que a planta está ativa               */
#ifdef DEBUG
int CONTRUIM =0 ;
#endif
int isserver = 0;
//===================== Cabeçalhos de Funções =====================//

void *threadGraph(void* args);
void *threadComm(void *port);
void atualiza_tabela(TPMENSAGEM msg);
int  verifica_tabela(TPMENSAGEM msg);
void responde_cliente(TPMENSAGEM msg, char ans[]);
void simulador();
void imprime_tabela();

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
  long int deltaT = 0;
  clockid_t clk_id = CLOCK_MONOTONIC_RAW;
  int attPlanta = 0 ;
  int attGraphTime = 0;
  int comando;

  //---- Verificação dos parametros 
  if (argc < 2 && argc!=3){  // chamada de programa + porta
    fprintf(stderr, "usage %s port isserver\n",argv[0]);
    exit(0);
  }else if (argc==2) { // se passar direto simulo servidor
    isserver = 1;
  }else{
    isserver = atoi(argv[2]);
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

  // Notificação de sucesso
  if (isserver){
    printf("Servidor iniciado para teste do servidor, parabens\n");
  }else{
    printf("Servidor iniciado para teste do cliente, parabens\n");
  }
 
  //---- Início nos timers
  clock_gettime(clk_id,&clkPlanta);
  clock_gettime(clk_id,&clkGraph);

  //---- LOOP principal
  while (OUT != 27){ // pressionando esq encerro o server PRECISO DAR JEITO DE FAZER A TEMPORIZACAO
    // Variavel OUT é caputara dela thread que está sempre operando -> COMM
    attPlanta = deltaTempo(TPLANTA,clkPlanta);
    attGraphTime = deltaTempo(TGRAPH,clkGraph);
    if (NOVAMENSAGEM && pthread_mutex_trylock(&mutexCOM)==0){
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
      simulador(); // NÂO COMENTAR ESSA BAGAÇA PQ TEM UM MANDRAKE LÁ NO MEIO DO THREAD COM QUE NÂO FOI CORRIGIDO
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

    if (attGraphTime && PLANTAATIVA){
      pthread_mutex_lock(&mutexGRAPH);
      ATTGRAPH =1; // aqui não importa o conteudo da msg
      attGraphTime = 0;
      clock_gettime(clk_id,&clkGraph);
      pthread_mutex_unlock(&mutexGRAPH);
    }
    
  }

  //---- Encerrando
  pthread_join(pthComm, NULL);
  pthread_join(pthGraph, NULL);
  //imprime_tabela();
  printf("Pacotes Perdidos + repetidos:\t%d\n", CONTRUIM );
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
  msg[0]='\0';
  printf("Servico de comunicação iniciado em thread\n");

  while (OUT != 27 && msg[0]!='\n'){ // pressionando esq encerro o server
    OUT = teclado(); 
    if(!flagNovaMsg){ // só altero n se leio 
      n = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&from, &fromlen);
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
        mensagem = analisarComando(msg,isserver);
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

void* threadGraph(void* args)
{
    int exit =0, ctrl=0, comando=0;
    int hasStarted = 0;
    int atualizarPlot = 0;
    #ifdef GRAPH
    Tdataholder *data;
    data = datainit(1000,500,150,120,(double)LVINIC,(double)0,(double)0);
    #endif
    pthread_mutex_lock(&mutexGRAPH); 
    #ifdef GRAPH
    datadraw(data,(double)PLANTASIM.tempo/1000.0,(double)PLANTASIM.nivel*100,(double)PLANTASIM.angIN,(double)PLANTASIM.angOUT);
    #endif
    pthread_mutex_unlock(&mutexGRAPH);
    printf("Thread Gráfica Iniciada\n");
    while(OUT != 27){
      if(INICIARGRAPH){  // se não comecei -> o importante é ver se tenho que começar
        if(pthread_mutex_trylock(&mutexGRAPH)==0){
          INICIARGRAPH = 0;
        // botar o limpar grafico e inicio aqui
          pthread_mutex_unlock(&mutexGRAPH);
        } 
      }
      if(!INICIARGRAPH){
        if(ATTGRAPH){
            pthread_mutex_lock(&mutexGRAPH);
            #ifdef GRAPH
            datadraw(data,(double)PLANTASIM.tempo/1000.0,(double)PLANTASIM.nivel,(double)PLANTASIM.angIN,(double)PLANTASIM.angOUT);
            #endif
            #ifndef GRAPH
            printf("PLANTA:\t%6ld\t%3d\t%3.3f\t%3.3f\n",PLANTASIM.tempo,PLANTASIM.nivel,PLANTASIM.angIN,PLANTASIM.angOUT);
            #endif
            ATTGRAPH = 0;
            pthread_mutex_unlock(&mutexGRAPH);
        }
      }
     // ISSO TEM Q IR POR CLIENTE
      /*
      angulo = controle(PLANTASIM.nivel);
      //printf("ang:%d\t",angulo);
      if(angulo==-100){
	      MENSAGEM.comando = C_S_CLOSE;
	      MENSAGEM.valor=-angulo;
      }else if(angulo==100){
	      MENSAGEM.comando = C_S_OPEN;
	      MENSAGEM.valor=angulo;
      }else{
	      MENSAGEM.comando = angulo;
      }
      if (PLANTASIM.nivel ==0) exit =1; 
     
      pthread_mutex_unlock(&mutexGRAPH);
      */
      #ifdef GRAPH
      quitevent();
      #endif
    }
    printf("FIM GRAFICO\n");
    return;
}


/**
*@brief Futura Thread que devera conter o simulador
*
**/
void simulador(){//simula simulador kkj NÂO APAGAR%
  MENSAGEM.comando=0;
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