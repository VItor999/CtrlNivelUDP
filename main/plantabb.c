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
#include "../headers/planta.h"


pthread_mutex_t mutexGRAPH= PTHREAD_MUTEX_INITIALIZER;

TPPLANTA PLANTASIM;

void* threadGraph(void* args);

int angulo = 0;
int flag_open = 0;
int flag_close = 0;

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
    int exit =0, ctrl=0, comando=0;
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
    datadraw(data,(double)PLANTASIM.tempo/1000.0,(double)PLANTASIM.nivel*100,(double)PLANTASIM.angIN,(double)PLANTASIM.angOUT);
    #endif
    atualizarPlanta (MENSAGEM,&PLANTASIM);
    MENSAGEM.comando = VAZIO;
    pthread_mutex_unlock(&mutexGRAPH);
    printf("INICIO GRAFICO");
    while(!exit){
      //waitms(10);
     
      //pthread_mutex_lock(&mutexGRAPH); 

      
      pthread_mutex_lock(&mutexGRAPH);
      #ifdef GRAPH
      datadraw(data,(double)PLANTASIM.tempo/1000.0,(double)PLANTASIM.nivel,(double)PLANTASIM.angIN,(double)PLANTASIM.angOUT);
      #endif
      atualizarPlanta (MENSAGEM,&PLANTASIM);
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
      #ifndef GRAPH
      printf("Atualizou:\t %3ld \t %3d \t%3d \t%3d\n",PLANTASIM.tempo,PLANTASIM.nivel,PLANTASIM.angIN,PLANTASIM.angOUT);
      #endif
      pthread_mutex_unlock(&mutexGRAPH);
      #ifdef GRAPH
      quitevent();
      #endif
    }
    printf("FIM GRAFICO");
    return;
}

int  controle(int level){
  int ang;
  if(level < REF && angulo != 100 && !flag_open){
    ang = 100;
    flag_open = 1;
    flag_close = 0;
  }else if(level > REF && angulo != -100 && !flag_close){
    ang = -100;
    flag_close = 1;
    flag_open = 0;
  }else{
    ang = 0;
  }
  return ang;
}
