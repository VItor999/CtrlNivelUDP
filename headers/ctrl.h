/**
*@file ctrl.h
*@author Lucas Esteves e Vitor Carvalho 
*@brief  Implementa algumas formas de controle possíveis 
*@version FINAL
*@date 2022-10-07
*
**/

//===================== Bibliotecas utilizadas =====================//
#include <stdio.h>
#include <stdlib.h>
#include "protocolo.h"

//====================== Definições efetuadas ======================//
#define UMAX 70                                   /* Valor do sinal de controle máximo incial*/
#define UINIC 20                                  /* Valor do sinal de controle inicial*/
#define REF 80                                    /* Valor da referencia*/
#define ANGINIC 50                                /* Valor do ângulo de aberutr inicial*/
#define LVINIC 40                                 /* Valor do nível da referência*/
#define KP 15                                     /* Valor do ganho proporcional */
#define KI 0.4                                    /* Valor do ganho integral */
#define KD 0                                      /* Valor do ganho derivativo */
#define CPID 1                                    /* Controlador PID*/
#define CBB  0                                    /* Controlador Bang-Bang*/
/**
*@brief Struc que contém os dados úteis para controle do processo
*
**/
typedef struct TPCONTROLE
{
    long int tempo;                               /* Tempo da rotina de controle*/
    float angulo;                                 /* Ângulo de abertura da válvula (0-100)*/
    int nivel;                                    /* Nivel atual */
    int flagEnvio;                                /* Status do envio de comando */
    TPMENSAGEM msg;
}TPCONTROLE;

typedef struct TPPID
{
    float e_1;
    float e_0;
    float p_0;
    float i_0;
    float i_1;
    float d_0;
    int ctrl_1;
}TPPID;

//===================== Cabeçalhos de Funções =====================//
int bang_bang(int level);
int ctrlPID (int level, TPPID *pid);
void starPID(TPPID *PID);

//#################################################################//
//########################   FUNÇÕES   ############################//
//#################################################################//

/**
*@brief Inicia os parâmetro do controle PID
*
*@param PID Estrutura do tipo PID que será inicializada
**/
void starPID(TPPID *PID){
  PID -> e_1 = 0;
  PID -> e_0 = LVINIC;
  PID -> p_0 = 40;  
  PID -> i_0 = 0;
  PID -> i_1 = 0;
  PID -> d_0 = 0;
  PID -> ctrl_1 = ANGINIC+UINIC;
}

/**
*@brief Rotina que efetua, de fato, um ciclo de atualização do controlador PID desenvolvido
*
*@param 1level Nível do tanque percebido pelo controlador
*@param pid  Endereço de uma estrutura PID
*@return int Retorno o valor de atuação da válvula. 
* O retorno é inteiro para já ser utilizado no protocolo
**/
int ctrlPID (int level, TPPID *pid){
  float e0,e1,i0,i1,p0,d0,ctrl1;
  ctrl1 = pid->ctrl_1;
  e1 = pid->e_0;
  i1 = pid->i_0;
  e0=(REF-level);
  //printf("Erro %f",e0 );
  p0 = e0*KP;
  
  i0 = i1 +KI*e0;  
  //if (e0>20) i0=0.5*i0;
  if (i0 > (REF-50)) i0 = (REF-50); // saturação da ação integral
   
  else if(i0 < (-100)) i0=-100; /// não deixe integrar negativamente
   //printf("I %f",i0 );
  d0= KD*(e0-e1);
  if (d0 > 20) i0 = 20; // saturação da ação derivativa
  //printf("d %f",d0 );
  
  float ctrl = p0 + i0 + d0;
  if (ctrl >100) ctrl=100;
  else if (ctrl<0) ctrl =0;
  int angulo = (int)(ctrl-ctrl1);

  pid->e_0 = e0;
  pid->p_0 = p0;
  pid->i_0 = i0;
  pid->d_0 = d0;
  pid->ctrl_1 = ctrl;
  pid->i_1 = i1;
  pid->e_1 = e1;
  //printf("ctrl %d\n",angulo);
  if (angulo > 100)angulo =100;
  else if (angulo<-100) angulo =-100;
  return angulo;
}


/**
*@brief Rotina de controle tipo Bang-bang (on/off)
*
*@param level Nível do tanque percebido pelo controlador
*@return int Retorno o valor de atuação da válvula. 
* O retorno é inteiro para já ser utilizado no protocolo
**/
int bang_bang(int level){
  static int flag_open=1;
  static int flag_close=0;
  int ang;
  if(level < REF && !flag_open){
    ang = UMAX;//100;
    flag_open = 1;
    flag_close = 0;
  }else if(level > REF && !flag_close){
    ang = -UMAX;//100;
    flag_close = 1;
    flag_open = 0;
  }else{
    ang = 0;
  }
  return ang;
}
