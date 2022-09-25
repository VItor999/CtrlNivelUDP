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

#define AUTO 1
//#define RAND 1
#define DEBUG 1
#define BUFFER_SIZE 100
#define TAM 10
#define TIMEOUT 100 // milissegundos
#define WAIT 80
#define LIN 50  
#define COL 3
#define NUM_COMM 100

int TABELA[LIN][COL]={0};
int POS[LIN];
int DISPONIVEL = LIN-1;
int LINHAATUAL = 0;

void error(const char *msg);
void simula_comm(char buffer[]);
char teclado();
void guarda_comando(TPMENSAGEM msg);
void confirma_comando(TPMENSAGEM msg);
void inicializa_pos();
void imprime_tabela();


int main(int argc, char *argv[]){
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    struct timespec start;
    struct timespec atual;
    long int deltaT = 0; 
    clockid_t clk_id;
    clk_id = CLOCK_MONOTONIC_RAW;
    char buffer[BUFFER_SIZE];
    char msg[BUFFER_SIZE];
    char out = '\0'; // tecla em que irei guardar a saida
    int flagAguardo = 0;
    //int perdido =0; 
    TPMENSAGEM mensagem_send, mensagem_recv;
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
    
    inicializa_pos();
    int CONT_IN = 0;
    int CONT_OUT = 0;
    while (teclado() != '\n' && out != '\n'){
        //waitms(WAIT);
        if (flagAguardo){ // estaria aguardando receber a resposta para continuar com uma nova escrita
            //printf("Continuo rodando\n");
            //printf(".");
            #ifdef AUTO
            clock_gettime(clk_id,&atual);
            if((atual.tv_sec-start.tv_sec)>0) {
                deltaT =(atual.tv_nsec/1000)+1000000-start.tv_nsec/1000;
            }
            else {
                deltaT =(atual.tv_nsec/1000)-start.tv_nsec/1000;
            }
            if (deltaT >= TIMEOUT*1000){
                flagAguardo = 0;
                printf("LOST");
                //perdido = 1;
            }
            #endif
        }else{// mandar uma  nova mensagem
            if(CONT_OUT<=NUM_COMM-1){
                bzero(buffer, BUFFER_SIZE);
                #ifdef AUTO
                simula_comm(buffer);
                #endif
                #ifndef AUTO
                fgets(buffer, BUFFER_SIZE, stdin);
                if (buffer[0]!='\n')buffer[strlen(buffer)-1]='\0';
                #endif
                out=buffer[0];
                n = write(sockfd, buffer, strlen(buffer));
                if (n < 0 && n != -1){ //
                    error("ERROR writing to socket");
                }else{
                    mensagem_send = analisarComando(buffer, 1);
                    clock_gettime(clk_id,&start);
                    //printf("start: %lld\n", start.tv_nsec);
                    guarda_comando(mensagem_send); // já escreve na tabela 
                    flagAguardo = 1;
                    CONT_OUT++;
                }
            }
        }
        bzero(buffer, BUFFER_SIZE);
        n = read(sockfd, buffer, BUFFER_SIZE-1);
        
        if (n < 0 && n != -1) // -1 quando não existe nada para ser lido
            error("ERROR reading from socket");
        else if (n > 0){ //NOVA MENSAGEM CAPTURADA
            strcpy(msg,buffer);
            mensagem_recv = analisarComando(buffer, 0);
            //printf("\t%s\n", msg);
            //printf("sent:%d\trecv:%d\n",mensagem_send.comando,mensagem_recv.comando+10);
            if((((mensagem_recv.comando+10) == mensagem_send.comando) && (mensagem_recv.sequencia == mensagem_send.sequencia)) || mensagem_recv.comando == C_S_ERRO){
                //printf("\tRECV: %s", msg);
                flagAguardo = 0;
                #ifndef AUTO
                printf("\n");
                #endif
            }
            /*else{
                printf("\n");
            }*/
            printf("\tRECV: %s", msg);
            confirma_comando(mensagem_recv); //tambem escreve na tabela apagando se recebeu erro 
        }
    }
    n = write(sockfd, "\n", 1);
    close(sockfd);
    printf("\n\nEncerrando cliente\n\n");
    imprime_tabela();
    return 0;
}

void guarda_comando(TPMENSAGEM msg){//escreve na tabela o comando ainda nao confirmado
    TABELA[POS[DISPONIVEL]][0]=msg.comando; //consulta vetor de posicoes disponiveis para guardar o comando
    TABELA[POS[DISPONIVEL]][1]=msg.sequencia;
    TABELA[POS[DISPONIVEL]][2]=msg.valor;
    if(POS[DISPONIVEL]==LINHAATUAL){ //acresce linhaatual se esta for a disponivel, do contrario mantem igual (pos até onde ja foi escrito)
        LINHAATUAL++;
    }
    POS[DISPONIVEL]=-1; //coloca valor invalido no vetor de posicoes para marcar
    DISPONIVEL--;   //diminui o numero de posicoes disponiveis
    #ifdef DEBUG
    printf("POSg:%d ",POS[DISPONIVEL]);
    printf("LAg:%d\t",LINHAATUAL);
    #endif
}

void confirma_comando(TPMENSAGEM msg){
    int i, encontrou=0;
    for(i=0;(i<LINHAATUAL && !encontrou);i++){ //percorre a tabela até achar o comando a ser confirmado
        if(msg.comando+10 == TABELA[i][0] && (msg.sequencia == TABELA[i][1])){// || msg.valor == TABELA[i][2])){//compara comando com seq ou valor
            TABELA[i][0]=VAZIO;
            TABELA[i][1]=VAZIO;
            TABELA[i][2]=VAZIO;
            if(i==(LINHAATUAL-1)){ //se a linha zerada é anterior à atual retrocede essa
                LINHAATUAL--;
                #ifdef DEBUG
                printf("\tLA--\t");
                #endif
            }
            DISPONIVEL++;
            POS[DISPONIVEL]=i;  //guarda no vetor a posição disponivel
            encontrou=1;
            #ifdef DEBUG
            printf("\tPOSc:%d ",POS[DISPONIVEL]);
            printf("LAc:%d\t",LINHAATUAL);
            #endif
        }/*else{
            printf("\tNot Found %d,%d(i:%d)\t",msg.comando,msg.sequencia,i);
        }*/
    }
}

void imprime_tabela(){
  int i;
  printf("TABELOSA\n");
  for(i=0; i<LINHAATUAL;i++){
    printf("(%d)\t%d, %d, %d\n",i,TABELA[i][0],TABELA[i][1],TABELA[i][2]);
  }
}

void inicializa_pos(){  //inicia vetor com valor ao contrario dos indices
    int i;
    for(i=0;i<LIN;i++){
        POS[i]=LIN-i-1;
    }
}

void error(const char *msg){
    perror(msg);
    exit(0);
}

void simula_comm(char buffer[]){
    char aux[TAM] = "\0";
    char STR_COMM[19] = "OpenValve#";
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
    printf("\n%s ",STR_COMM);
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