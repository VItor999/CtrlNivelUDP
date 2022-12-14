/**
*@file tempo.h
*@author Lucas Esteves e Vitor Carvalho 
*@brief Biblioteca com as funções de tempo
*@version FINAL
*@date 2022-09-21
*
**/

//===================== Bibliotecas utilizadas =====================//

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "kbhit.h"
//====================== Definições efetuadas ======================//
//#define DEBUG 1

#ifndef temporizacao
//===================== Cabeçalhos de Funções =====================//
void waitms(float twaitms);
int deltaTempo(int timeOut,struct timespec start);

//#################################################################//
//########################   FUNÇÕES   ############################//
//#################################################################//

/**
*@brief Função para esperar por milisegundos (qualquer valor entre 0-999)
*
*@param twaitms  Tempo de espera em milisegundos
**/
void waitms(float twaitms)
{
    long int deltaT =  0;
    int result;
    struct timespec inicial;
    struct timespec atual;
    // struct timespec: opera com dois long ints internamente 
    // o primeiro tv_sec := contem o valor em segundos 
    // o segundo  tv._nsec := contem o valor em nsegundos.
    //                        obviamente, vai até 999999999 nsegundos
    clockid_t clk_id;
    //  clk_id = CLOCK_REALTIME;
    clk_id = CLOCK_MONOTONIC_RAW;
    //  clk_id = CLOCK_BOOTTIME;
    //  clk_id = CLOCK_PROCESS_CPUTIME_ID;

    // int clock_gettime(clockid_t clk_id, struct timespec *tp);
    clock_gettime(clk_id, &inicial);
    clock_gettime(clk_id, &atual);
    #ifdef DEBUG
    printf("inicial.tv_sec: %lld\n", inicial.tv_sec);
    printf("inicial.tv_nsec: %ld\n", inicial.tv_nsec);
    #endif
    //char out ='\0';
    int twait =(int)twaitms*1000;
    while (deltaT < twait){
        if((atual.tv_sec-inicial.tv_sec)>0) {// se passou 1 segundo
            deltaT =(atual.tv_nsec/1000)+1000000-inicial.tv_nsec/1000;
        }
        else {
            deltaT =(atual.tv_nsec/1000)-inicial.tv_nsec/1000;
        }
        clock_gettime(clk_id, &atual);
        //out = teclado();
       // if(out =='\n' || out ==27){
       //     deltaT = twait+1;
       // }
    }
    #ifdef DEBUG
    printf("atual.tv_sec: %lld\n", atual.tv_sec);
    printf("atual.tv_nsec: %ld\n", atual.tv_nsec);
    #endif
    return;
}

/**
*@brief Função para informar se determinador delta T já ocorreu
*
*@param timeOut Tempo limite
*@param start  Relógio base iniciado externamente
*@return int 
**/
int deltaTempo(int timeOut,struct timespec start){
    long int deltaT = 0;
    struct timespec atual;
    int retorno =0;
    clockid_t clk_id = CLOCK_MONOTONIC_RAW;
    clock_gettime(clk_id,&atual);
    if((atual.tv_sec-start.tv_sec)>0) {
        deltaT = (atual.tv_nsec/1000)+1000000-start.tv_nsec/1000;
    }
    else {
        deltaT = (atual.tv_nsec/1000)-start.tv_nsec/1000;
    }
    if (deltaT >= (long int)timeOut*1000){
        //printf("Atual: %ld\n", atual.tv_nsec);
        retorno = 1;
    }
    return retorno;
}

#define temporizacao
#endif