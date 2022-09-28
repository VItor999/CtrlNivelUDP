#define GRAPH 1 
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

#define BUFFER_SIZE 100
#define TAM 10
#define TIMEOUT 100 // milissegundos
#define NUM_COMM 40
#define LIN 2*NUM_COMM  
#define COL 3

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
#ifdef DEBUG
int CONTRUIM = 0;
#endif 
//===================== Cabeçalhos de Funções =====================//

void error(const char *msg);
void simula_comm(char buffer[]);
void guarda_comando(TPMENSAGEM msg);
int confirma_comando(TPMENSAGEM msg);
void imprime_tabela();

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

void *threadComm(void *pcli){

    int sockfd, portno, rEnvio,rSend;
    struct sockaddr_in serv_addr;
    char  *chars;
    struct timespec start;
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
        if(NOVAESCRITA 
            && pthread_mutex_trylock(&mutexCOM)==0 
            && !esperandoResposta){
            bzero(buffer, BUFFER_SIZE);
            strcpy(buffer,BUFFER);
            rEnvio = write(sockfd, buffer, strlen(buffer));
            if (rEnvio < 0 && rEnvio != -1){ //
                error("ERROR writing to socket");
            }else{
                mensagem_send = analisarComando(buffer, 1);
                clock_gettime(clk_id,&start);     
                if(mensagem_send.comando == C_S_CLOSE || mensagem_send.comando == C_S_OPEN){
                    guarda_comando(mensagem_send); // já escreve na tabela 
                }
                esperandoResposta = 1;
                //CONT_OUT++;
                //NOVAESCRITA = 0;
            }
            pthread_mutex_unlock(&mutexCOM);
        }
        else if (esperandoResposta){ // estaria aguardando receber a resposta para continuar com uma nova escrita
        // dou time out 
            //printf("Continuo rodando\n");
            //printf(".");
            #ifdef AUTO
            if (deltaTempo(TIMEOUT,start)){
                esperandoResposta = 0;
                while(pthread_mutex_trylock(&mutexCOM)==0); //REVISAR ISSO ......
                //enquento não pega fica parado
                //se deu bom 
                NOVAESCRITA = 0;
                //CONT_OUT++;
                pthread_mutex_unlock(&mutexCOM);
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
            }
            if (pthread_mutex_trylock(&mutexCOM)==0){ // peguei o mutex 
                mensagem_recv = analisarComando(msg, 0);
                if(mensagem_recv.comando == C_C_CLOSE || mensagem_recv.comando == C_C_OPEN){
                    confirma = confirma_comando(mensagem_recv);
                }else{
                    confirma = 1;
                }    
                if (confirma){
                    obterInfo(&MENSAGEM,mensagem_recv); // se é algo valido mando pro mundo
                    NOVALEITURA = 1;  //notifica o mundo
                    if((((mensagem_recv.comando+10) == mensagem_send.comando) 
                        && (mensagem_recv.sequencia == mensagem_send.sequencia)) 
                        || mensagem_recv.comando == C_S_ERRO){
                        //printf("\tRECV: %s", msg);
                        esperandoResposta = 0;
                        #ifndef AUTO
                        printf("\n");
                        #endif
                    }   
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

int main(int argc, char *argv[]){
    system("clear");
    #ifdef DEBUG 
    srandom(time(NULL));
    #endif

    //---- Threads 
     pthread_t pthComm;       
  

    PCLIENTE pcliente;
    int sockfd;
 
     //---- Variáveis Auxiliares 
    int r;
    int CONT_OUT=0;

     //---- Verificação dos parametros 
    if (argc < 3)
    {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(0);
    }
 
    pcliente.porta = htons(atoi(argv[2]));
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    if (sockfd < 0){
        error("ERROR opening socket");
    }
    pcliente.charsv = argv[1];
  
    //---- Cria e  Testa a thread
    printf("Cliente Inicializado, parabens\n");
    r = pthread_create(&pthComm, NULL, threadComm, (void *)&pcliente);
    if (r){
        fprintf(stderr, "Error - pthread_create() return code: %d\n",  r);
        exit(EXIT_FAILURE);
    }
     //---- LOOP principal
 
    while (OUT != 27){ // pressionando esq encerro o server
        // Variavel OUT é caputara dela thread que está sempre operando -> COMM
        if (!NOVAESCRITA && pthread_mutex_trylock(&mutexCOM)==0){
            if(CONT_OUT==NUM_COMM){
                NOVAESCRITA = 0;
            }else{
                simula_comm(BUFFER); //escreve no buffer global para enviar
                NOVAESCRITA = 1;
                CONT_OUT++;
            }
            pthread_mutex_unlock(&mutexCOM);
        }
        if (NOVALEITURA && pthread_mutex_trylock(&mutexCOM)==0){
            #ifdef DEBUG
            printf("\tCOM %d\tSEQ %d\tVAL %d\n",MENSAGEM.comando,MENSAGEM.sequencia,MENSAGEM.valor);
            #endif
            NOVALEITURA = 0;
            pthread_mutex_unlock(&mutexCOM);
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

/**
*@brief Simula a comunicação
*
*@param buffer ponteiro/endereço para onde dever ser copiada a mensagem gerada
**/
void simula_comm(char buffer[]){
    char aux[TAM] = "\0";
    char STR_COMM0[20] = "GetLevel!";
    char STR_COMM1[19] = "OpenValve#";
    char STR_COMM2[20] = "CloseValve#";
   
    char STR_COMM[20];
    int resto = random()%3;
    if (resto==0){
        strcpy(STR_COMM,STR_COMM0);
    }
    else{
        if(resto==1){
            strcpy(STR_COMM,STR_COMM1);
        }
        else {
            strcpy(STR_COMM,STR_COMM2);
        }
        #ifdef RAND
        snprintf(aux, TAM, "%d", random()%1000);
        strcat(STR_COMM,aux);
        strcat(STR_COMM,TK);
        bzero(aux,TAM);
        snprintf(aux, TAM, "%d", random()%100);
        strcat(STR_COMM,aux);
        strcat(STR_COMM,ENDMSG);
        /*if(random()%11==0){
            waitms(999);
        }*/
        #endif
        #ifndef RAND
        static int i = 1;
        snprintf(aux, TAM, "%d",i);
        strcat(STR_COMM,aux);
        strcat(STR_COMM,TK);
        bzero(aux,TAM);
        snprintf(aux, TAM, "%d", i);
        strcat(STR_COMM,aux);
        strcat(STR_COMM,ENDMSG);
        i++;
        #endif
        
    }
    #ifdef DEBUG
        printf("\n%s ",STR_COMM);
    #endif
    strcpy(buffer,STR_COMM);
}
