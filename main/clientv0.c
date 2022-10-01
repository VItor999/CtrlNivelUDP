//#define GRAPH 1 
//===================== Bibliotecas utilizadas =====================//
#include <pthread.h>
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
#ifdef GRAPH
#include "../headers/graph.h"
#endif

//======================= Definições EXTRAS ========================//
#define AUTO 1
//#define RAND 1
#define DEBUG 1

//====================== Definições efetuadas ======================//
#define TPLANTA 10 //ms
#define UMAX 70
#define UINIC 20
#define BUFFER_SIZE 100
#define TAM 10
#define TIMEOUT 15 // milissegundos
//#define NUM_COMM 100
#define LIN 1000 //2*NUM_COMM  
#define COL 3
#define TCTRL 100
#define TGRAPH 50
#define REF 80
#define LVINIC 40
typedef struct TPCONTROLE
{
    long int tempo;
    float angulo;
    int nivel;
    int flagEnvio;
    TPMENSAGEM msg;
    /* data */
}TPCONTROLE;

//======================= Variáveis Globais  ======================//
pthread_mutex_t mutexCOM = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexGRAPH= PTHREAD_MUTEX_INITIALIZER;

char OUT = ' ';
TPMENSAGEM MENSAGEM;
int isserver = 0;
int TABELA[LIN][COL]={0};
int POS[LIN];
int DISPONIVEL = LIN-1;
int NOVALEITURA = 0;
int NOVAESCRITA = 0;
char BUFFER[BUFFER_SIZE]={0};
TPCONTROLE CTRL;
int INICIARGRAPH =0;
int ATTGRAPH =0;
#ifdef DEBUG
int CONTRUIM = 0;
#endif 
//===================== Cabeçalhos de Funções =====================//
void *threadGraphClient(void* args);
void *threadComm(void *pcli);
void error(const char *msg);
//void simula_comm(char buffer[]);
void guarda_comando(TPMENSAGEM msg);
int confirma_comando(TPMENSAGEM msg);
void imprime_tabela();
//void simula_ctrl(TPCONTROLE *ctrl);
void responde_servidor(TPMENSAGEM msg, char ans[]);
int bang_bang(int level);
void input_controle();
void output_controle();

/**
*@brief Parametros do cliente
*
**/
typedef struct PCLIENTE
{
    char *charsv;
    int porta;
    /* data */
}PCLIENTE;


//#################################################################//
//#########################    MAIN    ############################//
//#################################################################//

int main(int argc, char *argv[]){
    system("clear");
    #ifdef DEBUG 
    srandom(time(NULL));
    #endif

    //---- Threads 
     pthread_t pthComm;       
    pthread_t pthGraph;   

    //---- UDP
    PCLIENTE pcliente;
    int sockfd;
    
    //---- Variáveis Auxiliares 
    CTRL.flagEnvio = 1;
    CTRL.angulo =50;
    int r1,r2;
    int CONT_OUT=0;
    int flag_ctrl = 0;

    //---- Temporização 
    int attCtrl = 0;
    int attGraph = 0;
    struct timespec clkCtrl;
    struct timespec clkGraph;
    clockid_t clk_id = CLOCK_MONOTONIC_RAW;

     //---- Verificação dos parametros 
    if (argc < 3)
    {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(0);
    }
 
    //---- Captura de dado relevante para o UDP
    pcliente.porta = htons(atoi(argv[2]));
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    if (sockfd < 0){
        error("ERROR opening socket");
    }
    pcliente.charsv = argv[1];
  
    //---- Cria e  Testa a thread
    printf("Cliente Inicializado, parabens\n");
    r1 = pthread_create(&pthComm, NULL, threadComm, (void *)&pcliente);
    if (r1){
        fprintf(stderr, "Error - pthread_create() COMM return code: %d\n",  r2);
        exit(EXIT_FAILURE);
    }
     r2 =pthread_create( &pthGraph, NULL, threadGraphClient, NULL);
    if(r2)
    {
        fprintf(stderr, "Error - pthread_create() GRAPH return code: %d\n",  r2);
        exit(EXIT_FAILURE);
    }

     //---- LOOP principal
 
    clock_gettime(clk_id,&clkCtrl);
    clock_gettime(clk_id,&clkGraph);

    while (OUT != 27){ // pressionando esq encerro o server
        // Variavel OUT é caputara dela thread que está sempre operando -> COMM
        attCtrl = deltaTempo(TCTRL,clkCtrl);
        attGraph = deltaTempo(TGRAPH,clkGraph);
        if (!NOVAESCRITA){
            /*if(CONT_OUT==NUM_COMM){
                NOVAESCRITA = 0;
            }else{*/
                //simula_comm(BUFFER); //escreve no buffer global para enviar
                
            if(attCtrl){
                pthread_mutex_lock(&mutexGRAPH);
                output_controle();
                pthread_mutex_unlock(&mutexGRAPH);
                clock_gettime(clk_id,&clkCtrl);
                //NOVAESCRITA = 1;
                flag_ctrl = 1;
                //OUTROS COMANDOS QUE TB TEM QUE ESCREVRER, FAZER O IFS DESSAS PORRA
            }

                //CONT_OUT++;
            //}
            if(flag_ctrl && pthread_mutex_trylock(&mutexCOM)==0){
                NOVAESCRITA = 1;
                //printf("\tAAQUI/n");
                pthread_mutex_unlock(&mutexCOM);
                flag_ctrl = 0;
            }
        }
        if (NOVALEITURA){
            #ifdef DEBUG
            //printf("RECV: COM %d\tSEQ %d\tVAL %d\n",MENSAGEM.comando,MENSAGEM.sequencia,MENSAGEM.valor);
            #endif
            if(MENSAGEM.comando == C_C_GET 
            || MENSAGEM.comando == C_C_OPEN 
            || MENSAGEM.comando == C_C_CLOSE || MENSAGEM.comando == C_S_ERRO){
                //AQUI VAI UM LOCK DO MUTEX GRAFICO 
                //CHAMAR ATUALIZACAO DO CONTROLE AQUI
                pthread_mutex_lock(&mutexGRAPH);
                input_controle();
                pthread_mutex_unlock(&mutexGRAPH);
            }
            if(pthread_mutex_trylock(&mutexCOM)==0){    
                NOVALEITURA = 0;
                pthread_mutex_unlock(&mutexCOM);
            }
        }
        if(attGraph){//tem que atualizar
            if(pthread_mutex_trylock(&mutexGRAPH)==0){ 
                // peguei o mutex
                ATTGRAPH = 1;
                pthread_mutex_unlock(&mutexGRAPH);
                attGraph=0;
                clock_gettime(clk_id,&clkGraph);
            }
        }
       
    }
    //---- Encerrando
    pthread_join(pthComm, NULL);
    printf("Encerrando main\n\n");
    #ifdef DEBUG
    imprime_tabela();
    printf("Pacotes Perdidos + repetidos:\t%d\n", CONTRUIM );
    #endif
    return 0;
}

void* threadGraphClient(void* args)
{
    int exit =0, ctrl=0, comando=0;
    int hasStarted = 0;
    int atualizarPlot = 0;
    static long int tempo =0;
    #ifdef GRAPH
    Tdataholder *data;
    //editar posteriormente mas o plot é estado  tanque, válvula, referencia
    data = datainit(1000,500,150,120,(double)LVINIC,(double)50,(double)REF);
    #endif
    pthread_mutex_lock(&mutexGRAPH); 
    #ifdef GRAPH
    datadraw(data,(double)CTRL.tempo/1000.0,(double)CTRL.nivel,(double)CTRL.angulo,(double)REF);
    #endif
    pthread_mutex_unlock(&mutexGRAPH);
    printf("Thread Gráfica Iniciada\n");
    while(OUT != 27){
      if(INICIARGRAPH){  // se não comecei -> o importante é ver se tenho que começar
        if(pthread_mutex_trylock(&mutexGRAPH)==0){
          INICIARGRAPH = 0;
        // botar o limpar grafico e inicio aqui
	  tempo=0;
          pthread_mutex_unlock(&mutexGRAPH);
        } 
      }
      if(!INICIARGRAPH){
        if(ATTGRAPH){
	    tempo +=TGRAPH;
            pthread_mutex_lock(&mutexGRAPH);
            #ifdef GRAPH
            datadraw(data,(double)tempo/1000.0,(double)CTRL.nivel,(double)CTRL.angulo,(double)REF);
            #endif
            #ifndef GRAPH
            printf("CONTROLE:\tT-%3.3f \tN-%3.3f \t V-%3.3f R-%3.3f\n ",((double)tempo/1000.0),(double)CTRL.nivel,(double)CTRL.angulo,(double)REF);
            #endif
            ATTGRAPH = 0;
            pthread_mutex_unlock(&mutexGRAPH);
        }
      }
      #ifdef GRAPH
      quitevent();
      #endif
    }
    printf("FIM GRAFICO\n");
    return;
}

void input_controle(){
    int angulo;

    if(MENSAGEM.comando == C_C_GET){
        CTRL.nivel = MENSAGEM.valor;

        angulo = bang_bang(CTRL.nivel);
        if(angulo<0){
            CTRL.msg.comando = C_C_CLOSE;
            CTRL.angulo+=angulo;
            angulo = -angulo;
        }else if(angulo>0){
            CTRL.msg.comando = C_C_OPEN;
            CTRL.angulo+=angulo;
        }else{
            CTRL.msg.comando = C_C_OPEN;
        }
        CTRL.msg.valor = angulo;

    }else if(MENSAGEM.comando == C_C_OPEN || MENSAGEM.comando == C_C_CLOSE){
        CTRL.msg.comando = C_C_GET;
        CTRL.msg.valor = VAZIO;
    
    }else if (MENSAGEM.comando == C_C_START){
        printf("StartOK");
        INICIARGRAPH = 1;
        CTRL.tempo =0;
    }
    else{
        printf("ERRO ");//, PENSAR NO QUE FAZER, APENAS RETRANSMITE\n");
    }
    CTRL.flagEnvio = 1;
    printf("VALOR ANGULO%f\n",CTRL.angulo);
}

void output_controle(){
    static int i = 0;
    if(i==0){
        CTRL.msg.comando = C_C_START;
        CTRL.msg.sequencia = i;//random()%1000;
        CTRL.msg.valor = VAZIO;
        CTRL.flagEnvio = 1;
        // pensar o que fazer se perder um start
        
    }else if(i==1){
        CTRL.msg.comando = C_C_OPEN;
        CTRL.msg.sequencia = i;//random()%1000;
        CTRL.msg.valor = UINIC;
        CTRL.angulo +=  CTRL.msg.valor; // inicia em 100 dai
        CTRL.flagEnvio = 1;
        
    }
    
    if(CTRL.flagEnvio){
        bzero(BUFFER, BUFFER_SIZE);
        CTRL.msg.sequencia = i;
        responde_servidor(CTRL.msg, BUFFER);
        CTRL.flagEnvio = 0;
        //printf("Buffer %d: %s",i,BUFFER);
        i++;
        CTRL.tempo+=TPLANTA;
    }
}

int bang_bang(int level){
  static int flag_open=1;
  static int flag_close=0;
  int ang;
  if(level < REF && !flag_open){
    ang = UMAX;//100;
    flag_open = 1;
    flag_close = 0;
  }else if(level > REF && !flag_close){
    ang = -UMAX;//100;
    flag_close = 1;
    flag_open = 0;
  }else{
    ang = 0;
  }
  return ang;
}

void *threadComm(void *pcli){

    int sockfd, portno, rEnvio,rSend;
    struct sockaddr_in serv_addr;
    char  *chars;
    struct timespec start;
    struct timespec now;
    struct hostent *server;
    // relógio 
    clockid_t clk_id = CLOCK_MONOTONIC_RAW;
    char buffer[BUFFER_SIZE];
    char msg[BUFFER_SIZE];
    int esperandoResposta = 0;
    int leituraDisponivel = 0;
    int CONT_IN = 0;
    int CONT_OUT = 0;
    int confirma = 0;
    int flag_timeout = 0;
    TPMENSAGEM mensagem_send, mensagem_recv;
    portno = ((PCLIENTE *)pcli)->porta;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    if (sockfd < 0){
        error("ERROR opening socket");
    }
    chars =((PCLIENTE *)pcli)->charsv;
    server =  gethostbyname(chars);
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
    serv_addr.sin_port = portno;
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){ //binding
        error("ERROR connecting");
    }
    printf("Servico de comunicação iniciado em thread\n");
    while (OUT!=27 && OUT!='\n'){
        //waitms(WAIT);
        OUT = teclado(); 
        // Tentar escrever/ ENVIO
        if(NOVAESCRITA && !esperandoResposta && !leituraDisponivel && BUFFER[0]!='\0'){
            if(flag_timeout && pthread_mutex_trylock(&mutexCOM) == 0){
                bzero(BUFFER, BUFFER_SIZE);
                responde_servidor(CTRL.msg,BUFFER);
                pthread_mutex_unlock(&mutexCOM);
                flag_timeout = 0;
            }
            bzero(buffer, BUFFER_SIZE);
            strcpy(buffer,BUFFER);
            printf("Send: %s",buffer);
            //printf("%s\n", buffer);
            rEnvio = write(sockfd, buffer, strlen(buffer));
            if (rEnvio < 0 && rEnvio != -1){ //
                error("ERROR writing to socket");
            }else{
                mensagem_send = analisarComando(buffer, 1);   
                if(mensagem_send.comando == C_S_CLOSE || mensagem_send.comando == C_S_OPEN){
                    guarda_comando(mensagem_send); // já escreve na tabela 
                }
                esperandoResposta = 1;
                clock_gettime(clk_id,&start);  
                //printf("tempo: %ld\n",start.tv_nsec/1000000);
                //CONT_OUT++;
                //NOVAESCRITA = 0;
            }
            //pthread_mutex_unlock(&mutexCOM);
        }
        if (esperandoResposta){ // estaria aguardando receber a resposta para continuar com uma nova escrita
        // dou time out 
            //printf("Continuo rodando\n");
            //printf(".");
            #ifdef AUTO
            if (deltaTempo(TIMEOUT,start)){
                esperandoResposta = 0;
                //if(pthread_mutex_trylock(&mutexCOM)==0){ //REVISAR ISSO ......
                if(pthread_mutex_trylock(&mutexCOM)==0){
                //enquento não pega fica parado
                //se deu bom 
                    NOVAESCRITA = 0;
                    //clock_gettime(clk_id,&start);
                    //printf("NE: %d, ER: %d, LD: %d, tempo: %ld\n", NOVAESCRITA, esperandoResposta, leituraDisponivel,start.tv_nsec/1000000);
                    //CONT_OUT++;
                    flag_timeout = 1;
                    //clock_gettime(clk_id,&now);  
                    //printf("TIMEOUT: %ld, %ld\n", start.tv_nsec/1000000,now.tv_nsec/1000000);
                    pthread_mutex_unlock(&mutexCOM);
                }
            }
            #endif
        }
        //FAZER SEMPRE A LEITURA 
        if(!leituraDisponivel){//} && !esperandoResposta){
            //LEITURA OK +- se pegar mutex
            bzero(buffer, BUFFER_SIZE); //leia
            rSend = read(sockfd, buffer, BUFFER_SIZE-1); // tentar obter uma nova mensagem
        }
        if (rSend < 0 && rSend != -1) // -1 quando não existe nada para ser lido
            error("ERROR reading from socket");
        else if (rSend > 0){ //NOVA MENSAGEM CAPTURADA
            if (!leituraDisponivel){ //primeira vez que tenterei pegar o mutex
                strcpy(msg,buffer); 
                if (msg[0]=='\n'){ // desligar servidor 
                    OUT = 27;
                }
            //}
            
                mensagem_recv = analisarComando(msg, 0);
                if(mensagem_recv.comando == C_C_CLOSE || mensagem_recv.comando == C_C_OPEN){
                    confirma = confirma_comando(mensagem_recv);
                }else{
                    confirma = 1;
                }    
                if (confirma){
                    obterInfo(&MENSAGEM,mensagem_recv); // se é algo valido mando pro mundo
                    //NOVALEITURA = 1;  //notifica o mundo
                    if((((mensagem_recv.comando+10) == mensagem_send.comando) 
                        && (mensagem_recv.sequencia == mensagem_send.sequencia)) 
                        || mensagem_recv.comando == C_S_ERRO){
                        printf("\tRECV: %s\n", buffer);
                        esperandoResposta = 0;
                        #ifndef AUTO
                        printf("\n");
                        #endif
                    }   
                }
            }
            if (pthread_mutex_trylock(&mutexCOM)==0){ // peguei o mutex 
                if(confirma){
                    NOVALEITURA = 1;
                }
                NOVAESCRITA = 0;
                //CONT_OUT++;
                leituraDisponivel = 0;
                pthread_mutex_unlock(&mutexCOM); // libera o mutex
            }
            else{
                leituraDisponivel =1;
            }            
        }
    }
    OUT = 27;
    rSend = write(sockfd, "\n", 1);
    close(sockfd);
    printf("\n\nEncerrando threadCOM\n\n");
    return 0;
}

/**
*@brief Verifica posição livre na tabela de comandos enviados. 
* Retorna o indice da primeira posição livre encontrada
*@return int 
**/
int verificaPosicao(){
    int i,out=-1;
    for(i=0;i<LIN && out==-1 ;i++){
        if(TABELA[i][0]==VAZIO || TABELA[i][0]== 0){
            out=i;
        }
    }
    return out;
}

/**
*@brief Armazena uma mensagem enviada
*
*@param msg Mensagem enviada
**/
void guarda_comando(TPMENSAGEM msg){//escreve na tabela o comando ainda nao confirmado
    int pos = verificaPosicao();
    if (pos!=-1){
        TABELA[pos][0]=msg.comando; //consulta vetor de posicoes disponiveis para guardar o comando
        TABELA[pos][1]=msg.sequencia;
        TABELA[pos][2]=msg.valor;
    }
    // TODO pensar o que fazer quando minha tabela estourou
}

/**
*@brief Analisa a mesnsagem recebida de forma retira-lo da lista de comandos não confirmados
*    
*@param msg Mensagem de confirmação recebida
*@return int 
**/
int confirma_comando(TPMENSAGEM msg){
    int i, encontrou=0;
    for(i=0;(i<LIN && !encontrou);i++){ //percorre a tabela até achar o comando a ser confirmado
        if(msg.comando+10 == TABELA[i][0] && (msg.sequencia == TABELA[i][1])){// || msg.valor == TABELA[i][2])){//compara comando com seq ou valor
            TABELA[i][0]=VAZIO;
            TABELA[i][1]=VAZIO;
            TABELA[i][2]=VAZIO;
            encontrou=1;
        }/*else{
            printf("\tNot Found %d,%d(i:%d)\t",msg.comando,msg.sequencia,i);
        }*/
    }
    return encontrou;
}

/**
*@brief Imprime toda a tabela: Utilizado apenas para debug
*
**/
void imprime_tabela(){
  int i;
  printf("TABELOSA\n");
  for(i=0; i<LIN && TABELA[i][0] !=0 ;i++){
    if (TABELA[i][0] != VAZIO){
        CONTRUIM++;
        printf("(%d)\t%d, %d, %d\n",i,TABELA[i][0],TABELA[i][1],TABELA[i][2]);
    }
  }
}

void responde_servidor(TPMENSAGEM msg, char ans[]){
  #define TAM1 21
  char aux[TAM1]="\0";
  switch (msg.comando){
  case C_C_OPEN:
    strcat(ans,C_OPEN);
    strcat(ans,TK);
    snprintf(aux, TAM, "%d", msg.sequencia);
    strcat(ans,aux);
    strcat(ans,TK);
    snprintf(aux, TAM, "%d", msg.valor);
    strcat(ans,aux);
    break;
  case C_C_CLOSE:
    strcat(ans,C_CLOSE);
    strcat(ans,TK);
    snprintf(aux, TAM, "%d", msg.sequencia);
    strcat(ans,aux);
    strcat(ans,TK);
    snprintf(aux, TAM, "%d", msg.valor);
    strcat(ans,aux);
    break;
  case C_C_GET:
    strcat(ans,C_GETLV);
    break;
  case C_C_SET:
    strcat(ans,C_SETMAX);
    strcat(ans,TK);
    snprintf(aux, TAM, "%d", msg.valor);
    strcat(ans,aux);
    break;
  case C_C_COM:
    strcat(ans,C_COMTEST);
    break;
  case C_C_START:
    strcat(ans,C_START);
    break;
  
  default:
    strcat(ans,S_ERRO);
    break;
  }
  strcat(ans,ENDMSG);
}