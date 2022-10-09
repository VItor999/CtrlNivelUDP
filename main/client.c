/**
*@file clientv1.c
*@author Lucas Esteves e Vitor Carvalho 
*@brief  Código fonte do Cliente do trabalho 4 
*@version FINAL
*@date 2022-10-02
*
**/

//===================== Bibliotecas utilizadas =====================//
#define GRAPH 1 

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
#include "../headers/ctrl.h"
#ifdef GRAPH
#include "../headers/graph.h"
#endif

//======================= Definições EXTRAS ========================//
//#define AUTO 1
//#define RAND 1
//#define DEBUG 1

//====================== Definições efetuadas ======================//
#define TPLANTA 10                                /* Tempo de atualização do processo da planta em MS*/
#define TIMEOUT 15 // milissegundos               /* Tempo de TIMEOUT de um pacote em ms*/
#define TCTRL 100                                 /* Tempo de Atualização da thread de controle*/
#define TGRAPH 50                                 /* Tempo de Atualização da thread gráfica*/
//#define NUM_COMM 100
#define BUFFER_SIZE 100  //colocar no protocolo?  /* Tamanho do buffer*/
#define TAM 10                                    /* Tamanho do buffer auxiliar*/
#define LIN 5000 //2*NUM_COMM                     /* Número de linhas da tabela de comunicação*/ // futuramente colocar como dinâmica
#define COL 3                                     /* Número de colunas da tabela de comunição*/

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

//======================= Variáveis Globais  ======================//
// ---- MUTEXes 
pthread_mutex_t mutexCOM = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexGRAPH= PTHREAD_MUTEX_INITIALIZER;

// ---- Tipos Definidos
TPMENSAGEM MENSAGEM;
TPCONTROLE CTRL;
TPPID CTRLPID;

//---- Flags 
char OUT = ' ';                                 // Flag para Encerrar o programa
int NOVALEITURA = 0;                            // Indica a necessidade de nova leitura 
int NOVAESCRITA = 0;                            // Indica a necessidade de efetuar uma nova escrita 
int INICIARGRAPH =0;                            // Notifica o ínicio da thread Gráfica
int TIPOCONTROLE =0;                            // Indica tipo de controlador utilizado
//---- Com                                      
int TABELA[LIN][COL]={0};                       // Tabela com pacotes perdidos
char BUFFER[BUFFER_SIZE]={0};                   // Buffer global para escrita de mensagens

//---- Diversas 
#ifdef DEBUG
int CONTRUIM = 0;
#endif 

//===================== Cabeçalhos de Funções =====================//
void *threadGraphClient(void* args);
void *threadComm(void *pcli);
void error(const char *msg);
void guarda_comando(TPMENSAGEM msg);
int confirma_comando(TPMENSAGEM msg);
void responde_servidor(TPMENSAGEM msg, char ans[]);
void input_controle();
void output_controle();
void tryExit();
#ifdef DEBUG
void imprime_tabela();
#endif


//#################################################################//
//#########################    MAIN    ############################//
//#################################################################//

int main(int argc, char *argv[]){
    system("clear");                            // Limpa o console

    //---- Threads 
    pthread_t pthComm;       
    pthread_t pthGraph;   

    //---- UDP
    PCLIENTE pcliente;
    int sockfd;
    
    //---- Variáveis Auxiliares 
    CTRL.flagEnvio = 1;                       // Inicializa campo do struct para envio
    CTRL.angulo =ANGINIC;                     // Atribui angulo inicial
    int r1,r2;                                // Retornos para notificação de sucesso na criação de Threads 
    int flag_ctrl = 0;                        // Zera indicador de atualização do controle

    //---- Temporização 
    int attCtrl = 0;                          // Indica se está ou no instante de atualizar o controle;
    struct timespec clkCtrl;                  // Relógio do controle 
    clockid_t clk_id = CLOCK_MONOTONIC_RAW;   // Definido dessa forma para sempre ser progressivo
    clock_gettime(clk_id,&clkCtrl);           // Zera o tempo incial
    
    //---- Verificação dos parametros 
    if (argc < 3)
    {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(0);
    }
        
    //---- Captura de dado relevante para o UDP
    pcliente.porta = htons(atoi(argv[2]));
    pcliente.charsv = argv[1];
  
    //---- Cria e testa a Thread de Comunicação
    r1 = pthread_create(&pthComm, NULL, threadComm, (void *)&pcliente);
    if (r1){
        fprintf(stderr, "Error - pthread_create() COMM return code: %d\n",  r2);
        exit(EXIT_FAILURE);
    }
    
    //---- Cria e testa a Thread Gráfica
    r2 =pthread_create( &pthGraph, NULL, threadGraphClient, NULL);
    if(r2)
    {
        fprintf(stderr, "Error - pthread_create() GRAPH return code: %d\n",  r2);
        exit(EXIT_FAILURE);
    }
    printf("\nCliente Inicializado Corretamente, parabens\n"); 
    waitms(100);
    char opc ='\0';
    while (opc=='\0'){
        printf("Selecione o controlador desejado:\n\t (0) Bang-Bang \t(1) PID\nCaso deseje encerrar o programa, digite q\n");
        scanf("%c",&opc);
        switch ( opc)
        {
        case 'Q':
        case 'q':
            printf("\nEncerrado");
            OUT =27;
            break;
        case '0':
            system("clear");
            printf("\nControlador PID selecionado\n");
            printf("\nControlador Bang-Bang selecionado");
            
            TIPOCONTROLE = CBB;                         // Define o controle como Bang-Bang
            starPID(&CTRLPID);
            break;              
        case '1':
            system("clear");
            printf("\nControlador PID selecionado\n");
            printf("Executando o controle\n");    
            starPID(&CTRLPID);
            pthread_mutex_lock(&mutexCOM);           
            TIPOCONTROLE = CPID;                        // Define o controle como PID
            pthread_mutex_unlock(&mutexCOM);
            break;
        default:
            printf("\nERRO!! Selecione uma opção válida.");
            opc ='\0';
            break;
        }
    }
 
    
    //---- LOOP principal -> Roda a rotina de controle
    while (OUT != 27){                                  // Pressionando ESC encerro o cliente
        // Essa váriavel é atualizada pela Thread Gráfica (função TryExit)
        attCtrl = deltaTempo(TCTRL,clkCtrl);            // Se não existe nada para escrever
        if (!NOVAESCRITA){
            if(attCtrl){                                // Precisa atualizar o controle ??
                pthread_mutex_lock(&mutexGRAPH);
                output_controle();                      // Atualiza 
                pthread_mutex_unlock(&mutexGRAPH);
                clock_gettime(clk_id,&clkCtrl);
                //NOVAESCRITA = 1;
                flag_ctrl = 1;                          // Notifica necessidade     
                //OUTROS COMANDOS QUE TB TEM QUE ESCREVRER, FAZER O IFS DESSAS PORRA
            }
            if(flag_ctrl && 
                pthread_mutex_trylock(&mutexCOM)==0){ 
                NOVAESCRITA = 1;                       // Notifica (global) a necessidade de troca de mensagem
                //printf("\tAAQUI/n");
                pthread_mutex_unlock(&mutexCOM);
                flag_ctrl = 0;                 
            }
        }
        if (NOVALEITURA){                             // Existe algo novo para ser lido?
            #ifdef DEBUG
            //printf("RECV: COM %d\tSEQ %d\tVAL %d\n",MENSAGEM.comando,MENSAGEM.sequencia,MENSAGEM.valor);
            #endif
            if(MENSAGEM.comando == C_C_GET            // Caso algum desses comandos
            || MENSAGEM.comando == C_C_OPEN 
            || MENSAGEM.comando == C_C_CLOSE 
            || MENSAGEM.comando == C_S_ERRO
            || MENSAGEM.comando == C_C_START){  
                pthread_mutex_lock(&mutexGRAPH);    
                input_controle();                     // Escreva o Contole 
                pthread_mutex_unlock(&mutexGRAPH);
            }
   	    if(pthread_mutex_trylock(&mutexCOM)==0){    
                NOVALEITURA = 0;
                pthread_mutex_unlock(&mutexCOM);
            }
        }
    }
    //---- Encerrando
    pthread_join(pthComm, NULL);
    pthread_join(pthGraph, NULL);
    printf("\n===== Encerrando a Thread Principal  =====\n");
    #ifdef DEBUG
    imprime_tabela();
    printf("Pacotes Perdidos + repetidos:\t%d\n", CONTRUIM );
    #endif
    return 0;
}

void *threadComm(void *pcli){
    //---- Comunicaçao
    struct sockaddr_in serv_addr;
    struct hostent *server;

    //---- Temporização 
    struct timespec start;
    struct timespec now;
    clockid_t clk_id = CLOCK_MONOTONIC_RAW;
    struct timeval read_timeout;
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = uTIMEOUT;                 // Time Out

    //---- Auxiliares de comunicação
    char buffer[BUFFER_SIZE];
    char msg[BUFFER_SIZE];
    int esperandoResposta = 0;                       // Flag de aguardo de resposta
    int leituraDisponivel = 0;                       // Flag que indica que nova leitura estará disponível
    int confirma = 0;                                // Verifica o retorno de uma mensagem com número de sequência
    int flag_timeout = 0;                            // Indica time out do aguardo de uma resposta
    TPMENSAGEM mensagem_send, mensagem_recv;         // Estrutura com os dados de uma mensagem
    int sockfd, portno, rEnvio,rSend;
    char  *chars;

    //---- Interpreta argumentos da thread
    portno = ((PCLIENTE *)pcli)->porta;
    chars =((PCLIENTE *)pcli)->charsv;

    //---- Inicializa o socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);         // Socket UDP
    setsockopt(sockfd, SOL_SOCKET,                   // Soceket com time Out
                 SO_RCVTIMEO, &read_timeout,
                  sizeof read_timeout);
    if (sockfd < 0){
        error("ERROR opening socket");
    }
    
    //---- Captura o endereço de IP 
    server =  gethostbyname(chars);
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }
    
    //---- Estabelece a Conexão
    bzero((char *)&serv_addr, sizeof(serv_addr)); 
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
        (char *)&serv_addr.sin_addr.s_addr,
        server->h_length);
    serv_addr.sin_port = portno;
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){ //binding
        error("ERROR connecting");
    }

    //---- Notificação de sucesso
    printf("Servico de comunicação iniciado em thread\n");
    
    //---- LOOP de Comunicação
    while (OUT!=27 && OUT!='\n'){ // Sai em alguma dessas duas condições
        // Tentar escrever caso deva e não encontra-se esperando ou com algo já capturado
        if(NOVAESCRITA && !esperandoResposta && !leituraDisponivel && BUFFER[0]!='\0'){
            if(flag_timeout && pthread_mutex_trylock(&mutexCOM) == 0){
                bzero(BUFFER, BUFFER_SIZE);                 // Limpar buffer global
                responde_servidor(CTRL.msg,BUFFER);         // Escreve resposta
                pthread_mutex_unlock(&mutexCOM);            
                flag_timeout = 0;                           // Indica que deve ser iniciado a contagem de timeOUT
            }
            bzero(buffer, BUFFER_SIZE);                     // Limpa buffer local
            strcpy(buffer,BUFFER);
            rEnvio = write(sockfd, buffer, strlen(buffer)); // Verifica sucesso na escrita
            if (rEnvio < 0 && rEnvio != -1){ 
                printf("ERROR writing to socket");          // Notifica erro (pode ser catastrófico)
            }else{
                mensagem_send = analisarComando(buffer, 1); // Verifca o enviado  
                if(mensagem_send.comando == C_S_CLOSE 
                || mensagem_send.comando == C_S_OPEN){
                    guarda_comando(mensagem_send);          // Constata necessidade de armazená-lo
                }
                esperandoResposta = 1;                      // No aguardo de uma resposta
                clock_gettime(clk_id,&start);  
            }
        }
        if (esperandoResposta){                            // Se no Aguardo 
            if (deltaTempo(TIMEOUT,start)){                // Teste time Out
                esperandoResposta = 0;
                if(pthread_mutex_trylock(&mutexCOM)==0){   
                    NOVAESCRITA = 0;                       // Notifica o mundo é possível escrever
                    flag_timeout = 1;                      // Indica time Out
                    pthread_mutex_unlock(&mutexCOM);
                }
            }
        }
        
        if(!leituraDisponivel){                            // Se não existe entrada não interpretada
            bzero(buffer, BUFFER_SIZE);                 
            rSend = read(sockfd, buffer, BUFFER_SIZE-1);   // Tente efetuar uma obter uma nova mensagem
        }
        if (rSend < 0 && rSend != -1)           
            printf("ERROR reading from socket");
        else if (rSend > 0){                               // Caso de captura de mensagem com sucesso
            if (!leituraDisponivel){ 
                strcpy(msg,buffer); 
                if (msg[0]=='\n'){                         // Desligar caso a mensagem seja \n puro 
                    OUT = 27;                               
                }
                mensagem_recv = analisarComando(msg, 0);   // Analisa o que foi recebido
                if(mensagem_recv.comando == C_C_CLOSE      // Caso seja uma mensagem que precisa de confirmação  
                 || mensagem_recv.comando == C_C_OPEN){
                    confirma = confirma_comando(mensagem_recv); // Confirme
                }else{
                    confirma = 1;                          // Caso contrário siga em frente
                }    
                if (confirma){                          
                    obterInfo(&MENSAGEM,mensagem_recv);    // Captura dados
               
                    if((((mensagem_recv.comando+10) == mensagem_send.comando) 
                        && (mensagem_recv.sequencia == mensagem_send.sequencia)) 
                        || mensagem_recv.comando == C_S_ERRO 
                        || mensagem_recv.comando == C_S_START){
                        esperandoResposta = 0;
                    }   
                }
            }
            if (pthread_mutex_trylock(&mutexCOM)==0){ 
                if(confirma){                           
                    NOVALEITURA = 1;                       // Notifica outras Threads
                } 
                NOVAESCRITA = 0;                           // Habilita nova escrita  
                leituraDisponivel = 0;
                pthread_mutex_unlock(&mutexCOM);     
            }
            else{
                leituraDisponivel =1;
            }            
        }
    }
    //---- Encerra a comunicaçao
    OUT = 27;
    rSend = write(sockfd, "\n", 1);                        // Para encerrar o servidor simultâneamente
    close(sockfd);
    printf("\n==== Encerrando Thread de Comunicação ====\n");
}

void* threadGraphClient(void* args)
{
    //---- Auxiliares         
    int rodar = 0;                                         // Indica se o gráfico deve ser executado ou não
    int attGraph =0;                                       // Flag que indica quando atualizar o gráfico
    int taux;                                              // Auxiliar para reiniciar o gráfico
    
    //---- Temporização 
    static long int tempo = 0;                             // Tempo de simulação 
    struct timespec clkGraph;                              // Relógio utilizado
    clockid_t clk_id = CLOCK_MONOTONIC_RAW;                // Monotonic para sempre avançar


    //---- Inicialização 
    clock_gettime(clk_id,&clkGraph);                       // Captura do tempo
    #ifdef GRAPH
    Tdataholder *data;                               
    data = datainit(640,480,150,120,(double)LVINIC,(double)50,(double)REF);
    #endif
    pthread_mutex_lock(&mutexGRAPH); 
    #ifdef GRAPH
    datadraw(data,(double)CTRL.tempo/1000.0,(double)CTRL.nivel,(double)CTRL.angulo,(double)REF);
    #endif
    pthread_mutex_unlock(&mutexGRAPH);

    //---- Notificação de sucesso
    printf("Serviço Gráfico inciado em Thread\n");

    //---- LOOP Gráfico principal
    while(OUT != 27){
        if(INICIARGRAPH || rodar ==-1){                    // Caso não tenha iniciado ou não deva executar
            if(pthread_mutex_trylock(&mutexGRAPH)==0){      
                INICIARGRAPH = 0;                              
                rodar = 1;                                 // Habilita a execução
                #ifdef GRAPH
                Restart(640,480,150,120,(double)CTRL.nivel,(double)CTRL.angulo,(double)REF,data);
                #endif
                pthread_mutex_unlock(&mutexGRAPH);
            } 
        } 
        if(rodar==1){                                      // Se deve executar  
            attGraph = deltaTempo(TGRAPH,clkGraph);        // Atualiza status do contador
            if(attGraph){
                tempo += TGRAPH;                           // Aumenta o passo
                taux = tempo%150000;                       // Verifica se chegou a fim do gráfico (150s)
                pthread_mutex_lock(&mutexGRAPH);
                #ifdef GRAPH
                datadraw(data,(double)taux/1000.0,(double)CTRL.nivel,(double)CTRL.angulo,(double)REF);
                #endif
                #ifndef GRAPH
                printf("CONTROLE:\tT-%3.3f \tN-%3.3f \t V-%3.3f R-%3.3f\n ",((double)tempo/1000.0),(double)CTRL.nivel,(double)CTRL.angulo,(double)REF);
                #endif
                pthread_mutex_unlock(&mutexGRAPH);
                if ((taux)==0){                         // Se chegou ao fim do gráfico                  
                    rodar = -1;                         // Forçar a limpeza do gráfico
                }
                tryExit();                              // A cada 50 ms verifica se o User deseja sair 
                clock_gettime(clk_id,&clkGraph);        // Atualiza o relógio
            }
        }
        else{
            tryExit();                                  // Sempre Verifica se o usuário deseja sair             
        }
    }
    printf("\n======== Encerrando Thread Gráfica =======\n");
}

/**
*@brief Verifica se o usuário deseja encerrar o programa
*
**/
void tryExit(){
    //OUT = teclado(); 
    #ifdef GRAPH
    quitevent(&OUT);
    #endif
}

/**
*@brief Analisa uma mensagem de entrada e, caso necessário atualiza o controle
*
**/
void input_controle(){
    int angulo;
    if (MENSAGEM.comando == C_C_START){
        //printf("StartOK");
        INICIARGRAPH = 1;
        CTRL.tempo =0;
    }
    else if(MENSAGEM.comando == C_C_GET){
        CTRL.nivel = MENSAGEM.valor;
        if(TIPOCONTROLE==CBB)angulo = bang_bang(CTRL.nivel);
        else if (TIPOCONTROLE=CPID)angulo = ctrlPID(CTRL.nivel,&CTRLPID);
        if(angulo<0){
            CTRL.msg.comando = C_C_CLOSE;
            CTRL.angulo+=angulo;
            if (CTRL.angulo < 0) CTRL.angulo =0; 
            angulo = -angulo;
        }else if(angulo>0){
            CTRL.msg.comando = C_C_OPEN;
            CTRL.angulo+=angulo;
            if (CTRL.angulo >= 100) CTRL.angulo =100; 
        }else{
            CTRL.msg.comando = C_C_OPEN;
        }
        CTRL.msg.valor = angulo;

    }else if(MENSAGEM.comando == C_C_OPEN || MENSAGEM.comando == C_C_CLOSE){
        CTRL.msg.comando = C_C_GET;
        CTRL.msg.valor = VAZIO;
    
    }else{
        printf("ERRO ");
    }
    CTRL.flagEnvio = 1;
}

/**
*@brief Define o que Será enviado
*
**/
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
        CTRL.angulo +=  CTRL.msg.valor; 
        CTRL.flagEnvio = 1;
        
    }
    
    if(CTRL.flagEnvio){
        bzero(BUFFER, BUFFER_SIZE);
        CTRL.msg.sequencia = i;
        responde_servidor(CTRL.msg, BUFFER);
        CTRL.flagEnvio = 0;
        //printf("Buffer %d: %s",i,BUFFER);
        i++;
        CTRL.tempo+=TCTRL;
    }
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
*@brief Analisa a mesnsagem recebida de forma retirá-lo da lista de comandos não confirmados
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
        }
    }
    return encontrou;
}

/**
*@brief Imprime toda a tabela: Utilizado apenas para debug
*
**/
#ifdef DEBUG
void imprime_tabela(){
  int i;
  printf("Tabela\n");
  for(i=0; i<LIN && TABELA[i][0] !=0 ;i++){
    if (TABELA[i][0] != VAZIO){
        CONTRUIM++;
        printf("(%d)\t%d, %d, %d\n",i,TABELA[i][0],TABELA[i][1],TABELA[i][2]);
    }
  }
}
#endif

/**
*@brief Cria a mensagem que será transmitida de fato ao servidor 
*
*@param msg Estrutura que contém a informação a ser transmitida
*@param ans Onde deve ser armazenada a mensagem (BUFFER que será transmitido)
**/
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
