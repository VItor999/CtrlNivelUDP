/**
*@file planta.h
*@author Lucas Esteves e Vitor Carvalho 
*@brief Arquivo auxiliar com as definições do processo a ser simulado
*@version 0.1
*@date 2022-10-02
*
**/

//===================== Bibliotecas utilizadas =====================//
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "protocolo.h"

#ifndef planta

//====================== Definições efetuadas ======================//
#define PARAM 0                         /* Comamndo para atualizar somente os parâmetros da planta*/
#define CICLO 1                         /* Comando para efetuar somente um ciclo do processo*/
#define PARAMCICLO 2                    /* Comando para atualizar parâmetros e efetuar um ciclo do porcesso*/
#define LVINIC 40                       /* Nível incial do tanque */
#define INANGL 50                       /* Ângulo inicial da válvula*/  
#define REF 80                          /* Nível de referencia*/
#define TPLANTA 10 //ms                 /* Taxa de atualização do processo*/
#define MAXINIC 80                      /* Valor inicial do máximo*/

/**
*@brief Estrutura que define os elementos necessários para simular o processo
*
**/
typedef struct TPPLANTA
{
    long int tempo;                     /* Tempo em segundos*/
    int nivel;                          /* Nivel em porcentagem 0 -100*/
    float angIN;                        /* Ângulo da válvula de entrada*/ 
    float angOUT;                       /* Ângulo da válvula de saída*/
    int max;                            /* Valor do máximo*/
}TPPLANTA;


//===================== Cabeçalhos de Funções =====================//
float outAngle(long int T);
void atualizarPlanta (TPMENSAGEM msg,TPPLANTA *PLANTA,int opc);


//#################################################################//
//########################   FUNÇÕES   ############################//
//#################################################################//

/**
*@brief Atualiza o planta/simulador
*
*@param msg mensagem contendo as informações relevantes
*@param PLANTA Processo simulado (recebe o endereço)
*@param opc Opção para atualizar params de PLANTA (0) PARAM, efetuar um ciclo (1) CICLO ou efetuar ambos os processos(2) PARAMCICLO;
**/
void atualizarPlanta (TPMENSAGEM msg,TPPLANTA *PLANTA,int opc){
 //inputs iniciais
 
    static float influx = 0;     // inicia com a valvula Aberta em 50%
    static float outflux = 0; 
    static float delta = 0;     // inteiro o -100
    static float inAngle = INANGL;         //0-100
    static float level = ((float)LVINIC)/100.0;
    long int dT = TPLANTA;// passo do processo em ms;
    static long int T= 0;
    if(opc == PARAM || opc == PARAMCICLO){ // só atualiza
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
           PLANTA->max = msg.valor; // valor de 0-100
           //printf("QUEM È O MAX %d",PLANTA->max);
        }
        if(msg.comando == C_S_START){
            influx = 0;     // inicia com a valvula Aberta em 50%
            outflux = 0; 
            delta = 0;     // inteiro o -100
            inAngle = INANGL;         //0-100
            level = ((float)LVINIC)/100.0;
            dT = TPLANTA;// passo do processo em ms;
            T= 0;
            level = ((float)LVINIC)/100.0;
            PLANTA->max = MAXINIC;
            PLANTA->angIN  = (float)inAngle;
            PLANTA->nivel = (int)level;     
            //printf("ZERADITO");      
        }
    }
    if(opc == CICLO || opc == PARAMCICLO){ // 1 simula  de fato
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
        //printf("inAngle:%d\n",inAngle);
        // definir T -> tempo depois do start
        T= T+TPLANTA;
        influx = 1 * sin(M_PI / 2 * inAngle / 100);
        outflux = (((float)PLANTA->max) / 100) * (level / 1.25 + 0.2) * sin(M_PI / 2 * outAngle(T) / 100);
        level = level + 0.00002 * dT * (influx - outflux);

        if (level <0){
            level = 0;
        }
        else if(level >1){
            level=1;
        }
    }
    PLANTA->nivel = (int)(level*100);
    PLANTA->angIN = (float)inAngle;
    PLANTA->angOUT = (float)outAngle(T);
    PLANTA->tempo = T;
}

/**
*@brief Função que define o angulo de abertura da válvula de perturbação
*
*@param T Tempo atual
*@return float angulo da perturbação
**/
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
#define planta
#endif