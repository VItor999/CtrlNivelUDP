/**
*@file time.c
*@author Lucas Esteves e Vitor Carvalho 
*@brief Biblioteca com as funções de tempo
*@version 0.1
*@date 2022-09-21
*
**/

//===================== Bibliotecas utilizadas =====================//

#include <stdio.h>
#include <time.h>
#include <stdlib.h>

//====================== Definições efetuadas ======================//
//#define DEBUG 1

#ifndef temporizacao


//#################################################################//
//########################   FUNÇÕES   ############################//
//#################################################################//

/**
*@brief Função para esperar por milisegundos (qualquer valor entre 0-999)
*
*@param twaitms  TTempo de espera em milisegundos
**/

void waitms(int twaitms)
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

    while (deltaT < twaitms*1000){
        if((atual.tv_sec-inicial.tv_sec)>0) {// se passou 1 segundo
            deltaT =(atual.tv_nsec/1000)+1000000-inicial.tv_nsec/1000;
        }
        else {
            deltaT =(atual.tv_nsec/1000)-inicial.tv_nsec/1000;
        }
        clock_gettime(clk_id, &atual);
        
    }
    #ifdef DEBUG
    printf("atual.tv_sec: %lld\n", atual.tv_sec);
    printf("atual.tv_nsec: %ld\n", atual.tv_nsec);
    #endif
}
#define temporizacao
#endif