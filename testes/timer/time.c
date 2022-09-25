#include <stdio.h>
#include <time.h>
#include <stdlib.h>
int main(int argc, char **argv)
{
    long int deltaT =  0;
    if (argc <2){
        printf("Erro, passe um argumento do tempo de delay em ms\n");
        return 1;
    } 
    int twait = atoi(argv[1]);
    int result;
    struct timespec inicial;
    struct timespec atual;
    // struct timespec: opera com dois long ints internamente 
    // o primeiro tv_sec := contem o valor em segundos 
    // o segundo  tv._nsec := contem o valor em nsegundos.
    //                        obviamente, vai até 999999999 nsegundos  --> 000000000
    clockid_t clk_id;
    system("clear");
    //  clk_id = CLOCK_REALTIME;
    clk_id = CLOCK_MONOTONIC_RAW;
    //  clk_id = CLOCK_BOOTTIME;
    //  clk_id = CLOCK_PROCESS_CPUTIME_ID;

    // int clock_gettime(clockid_t clk_id, struct timespec *tp);
    clock_gettime(clk_id, &inicial);
    clock_gettime(clk_id, &atual);
    printf("inicial.tv_sec: %lld\n", inicial.tv_sec);
    printf("inicial.tv_nsec: %ld\n", inicial.tv_nsec);
    while (deltaT < twait*1000){
        if((atual.tv_sec-inicial.tv_sec)>0) {// se passou 1 segundo
            deltaT =(atual.tv_nsec/1000)+1000000-inicial.tv_nsec/1000; // todos em ms
            // 300 
        }
        else {
            deltaT =(atual.tv_nsec/1000)-inicial.tv_nsec/1000;
        }
        clock_gettime(clk_id, &atual);
        
    }
    printf("atual.tv_sec: %lld\n", atual.tv_sec);
    printf("atual.tv_nsec: %ld\n", atual.tv_nsec);
    printf("deltaT: %ld\n", deltaT);
}