#define GRAPH 1 

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "../headers/tempo.h"
#include "../headers/protocolo.h"
#include "../headers/kbhit.h"
#include <math.h>
#ifdef GRAPH
#include "../headers/graph.h"
#endif
#define DEBUG 1

#define LVINIC 40
#define INANGL 50
#define DELTAT 1000 // em ms

pthread_mutex_t mutexGRAPH= PTHREAD_MUTEX_INITIALIZER;

typedef struct TPPLANTA
{
    long int tempo; /*Tempo em segundos*/
    int nivel;      /*Nivel em porcentagem*/
    int angIN;      /*Ângulo da válvula de entrada*/ 
    int angOUT;     /*Ângulo da válvula de saída*/
}TPPLANTA;

TPPLANTA PLANTA;

float outAngle(long int T);
void* threadGraph(void* args);
void atualizarPlanta (TPMENSAGEM msg);

int main()
{
    pthread_t pthGraph; 
    int r1;
    char out ='\0';
    r1 =pthread_create( &pthGraph, NULL, threadGraph, NULL);
     if(r1)
    {
      printf("Thread creation failed: %d\n", r1);
    }
   while(out!=27  && out !='\n'){
        out = teclado();
   }
   pthread_join(pthGraph, NULL);
   return 0;
}

void* threadGraph(void* args)
{
    int exit =0;
    #ifdef GRAPH
    Tdataholder *data;
    data = datainit(1000,500,150,120,(double)LVINIC,(double)0,(double)0);
    #endif
    while(pthread_mutex_trylock(&mutexGRAPH)==0){
        printf(".");
    } 
    TPMENSAGEM MENSAGEM;
    MENSAGEM.comando = C_S_START;
    MENSAGEM.valor = VAZIO;
    MENSAGEM.sequencia =VAZIO;
    #ifdef GRAPH
    datadraw(data,(double)PLANTA.tempo/1000.0,(double)PLANTA.nivel*100,(double)PLANTA.angIN,(double)PLANTA.angOUT);
    #endif
    atualizarPlanta (MENSAGEM);
    MENSAGEM.comando = VAZIO;
    pthread_mutex_unlock(&mutexGRAPH);
    printf("INICIO GRAFICO");
    while(!exit){
      //waitms(10);
     
      //pthread_mutex_lock(&mutexGRAPH); 

      
      pthread_mutex_lock(&mutexGRAPH);
      #ifdef GRAPH
      datadraw(data,(double)PLANTA.tempo/1000.0,(double)PLANTA.nivel,(double)PLANTA.angIN,(double)PLANTA.angOUT);
      #endif
      atualizarPlanta (MENSAGEM);
      if (PLANTA.nivel ==0) exit =1; 
      #ifndef GRAPH
      printf("Atualizou:\t %3ld \t %3d \t%3d \t%3d\n",PLANTA.tempo,PLANTA.nivel,PLANTA.angIN,PLANTA.angOUT);
      #endif
      pthread_mutex_unlock(&mutexGRAPH);
      //quitevent();
    }
    printf("FIM GRAFICO");
    return;
}

void atualizarPlanta (TPMENSAGEM msg){
 //inputs iniciais
 
    static float influx = 50;     // inicia com a valvula Aberta em 50%
    static float outflux = 0; 
    static int MAX = 50;       // setar isso como ??
    static float delta = 0;     // inteiro o -100
    static float inAngle = 0;         //0-100
    static float level = ((float)LVINIC)/100;
    long int dT = DELTAT;// passo do processo em ms;
    int NIVEL = LVINIC;
    static long int T= 0;
    if (msg.comando == C_S_OPEN) //open
    {
        delta += msg.valor;
    }
    if (msg.comando == C_S_CLOSE) //close
    {
        delta -= msg.valor;
    }
    if (msg.comando == C_S_SET)
    {
        MAX = msg.valor; // valor de 0-100
    }
    if (delta > 0)  //abrir 
    {
        if (delta < 0.01 * dT)
        {
            inAngle +=  delta;
            delta = 0 ;
        }
        else {
            inAngle+=  0.01 * dT;
            delta -= 0.01 * dT;
        }
    }
    else if (delta < 0) // fechar
    {
        if (delta > -0.01 * dT)
        {
            inAngle +=delta;
            delta = 0 ;
        }
        else{
            inAngle -= 0.01 * dT;
            delta += 0.01 * dT;
        } 
    }
    // definir T -> tempo depois do start
    T= T+DELTAT;
    influx = 1 * sin(M_PI / 2 * inAngle / 100);
    outflux = (((float)MAX) / 100) * (level / 1.25 + 0.2) * sin(M_PI / 2 * outAngle(T) / 100);
    printf("LA%f",level);
    level = level + 0.00002 * dT * (influx - outflux);
    printf("LF%f",level);
    if (level <=0){
        level = 0;
    }
    PLANTA.nivel = (int)(level*100);
    PLANTA.angIN = (int)inAngle;
    PLANTA.angOUT = (int)outAngle(T);
    PLANTA.tempo = T;
}

float outAngle(long int T)
{
    if (T <= 0)
    {
        return 50;
    }
    if (T < 20000)
    {
        return (50 + T / 400);
    }
    if (T < 30000)
    {
        return 100;
    }
    if (T < 50000)
    {
        return (100 - (T - 30000) / 250);
    }
    if (T < 70000)
    {
        return (20 + (T - 50000) / 1000);
    }
    if (T < 100000)
    {
        return (40 + 20 * cos((T - 70000) * 2 * M_PI / 10000));
    }
    return 100;
}