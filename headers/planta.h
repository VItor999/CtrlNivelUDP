/**
*@file planta.h
*@author Lucas Esteves e Vitor Carvalho 
*@brief Arquivo auxiliar com as definições do processo a ser simulado
*@version FINAL
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
    // Variáveis estáticas para eveitar ter que recriá-las, uma vez que essa função será 
    // chamada com base no tempo de atualização do processo 
    static float influx = 0;          
    static float outflux = 0; 
    static float delta = 0;                                     // Define a dinâmica da Válvula 
    static float inAngle = INANGL;                              // Inicia com a valvula Aberta em 50% 0-100
    static float level = ((float)LVINIC)/100.0;                 // Nivel incial (0-1) -> valor em porcentagem convertid
    static long int T= 0;       
    if(opc == PARAM || opc == PARAMCICLO){                      // Para somente atualizar um parametro
        if (msg.comando == C_S_OPEN)                            // Abre a válvula
        {       
            delta += msg.valor;     
        }       
        if (msg.comando == C_S_CLOSE)                           // Fecha a válvula
        {       
            delta -= msg.valor;     
        }       
        if (msg.comando == C_S_SET)                             // Define o valor máximo do fluxo de saída
        {       
           PLANTA->max = msg.valor;                             // valor de 0-100
        }           
        if(msg.comando == C_S_START){                           // Reinicia o processo 
            influx = 0;                                         // Inicia com fluxo 0
            outflux = 0;        
            delta = 0;          
            T= 0;                           
            inAngle = INANGL;                                   // Inicia com o angulo inicial ()
            level = ((float)LVINIC)/100.0;      
            level = ((float)LVINIC)/100.0;      
            PLANTA->max = MAXINIC;      
            PLANTA->angIN  = (float)inAngle;        
            PLANTA->nivel = (int)level;             
        }       
    }                                                           
    if(opc == CICLO || opc == PARAMCICLO){                      // Efetua um ciclo de simulação de fato
        if (delta > 0)                                          // Caso deva Abrir 
        {       
            if (delta < 0.01 * TPLANTA)                         // Encerra a dinâmica da válvula
            {       
                inAngle +=  delta;      
                delta = 0 ;     
            }       
            else {                                              // Gera a dinâmica da válvula
                inAngle +=  0.01 * TPLANTA;     
                delta -= 0.01 * TPLANTA;        
            }       
        }       
        else if (delta < 0)                                     // Caso deva Fechar 
        {       
            if (delta > -0.01 * TPLANTA)                        // Encerra a dinâmica da válvula
            {       
                inAngle +=delta;        
                delta = 0 ;     
            }       
            else{                                               // Efetua a dinâmica da válvula 
                inAngle -= 0.01 * TPLANTA;      
                delta += 0.01 * TPLANTA;        
            }       
        }       
        T= T+TPLANTA;                                           // Soma o passo
        influx = 1 * sin(M_PI / 2 * inAngle / 100);             // Calcula o fluxo de entrada
        // Calcula o fluxo de saída         
        outflux = (((float)PLANTA->max) / 100) * (level/1.25 + 0.2) * sin(M_PI/2 * outAngle(T)/100);
        level = level + 0.00002 * TPLANTA * (influx - outflux); // Atualiza o nível 
         
        if (level <0){                                         // Saturação do nível negativo
            level = 0;
        }
        else if(level >1){                                     // Saturação do nível positivo
            level=1;
        }
    }
    // Atualização dos valores no struc de entrada 
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