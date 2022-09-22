/**
*@file protocolo.h
*@author Lucas Esteves e Vitor Carvalho 
*@brief Biblioteca própria auxiliar para a definição do protocolo de comunição Utilizado.
*@version 0.1
*@date 2022-09-18
*
**/

//===================== Bibliotecas utilizadas =====================//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef protocolo

//====================== Definições efetuadas ======================//

//#define DEBUG 1
// Defines do Protocolo próprio escrito do ponto de vista do cliente (COMANDOS AZUIS)
// O_XXXX output qualquer comando DE SAIDA (AZUL NO PDF)
// comando#XX#YYY!
//		  12345678		

//-------------- Comandos enviamos pelo cliente -------------------//

#define C_OPEN "OpenValve" 		/** String para identificar comandos: OpenValve#123#999! **/	
#define C_CLOSE "CloseValve"	/** String para identificar comandos: CloseValve#13#999! **/  		  
#define C_GETLV "GetLevel"		/** String para identificar comandos: GetLevel!		 	 **/
#define C_SETMAX "SetMax" 		/** String para identificar comandos: SetMax#123!		 **/	
#define C_COMTEST "CommTest" 	/** String para identificar comandos: CommTest!		 	 **/
#define C_START "Start"			/** String para identificar comandos: Start!  			 **/	

//-------------- Comandos enviamos pelo servidor ------------------//

#define S_OPEN "Open"     		/** String para identificar comandos: Open#123!			 **/
#define S_CLOSE "Close"  		/** String para identificar comandos: Close#123!		 **/
#define S_GETLV "Level" 		/** String para identificar comandos: Level#999!  		 **/
#define S_SETMAX "Max" 			/** String para identificar comandos: Max#999!			 **/
#define S_ERRO "Err" 			/** String para identificar comandos: Err!   			 **/ 
#define S_COMTEST "Comm" 		/** String para identificar comandos: Comm#OK!        	 **/
#define S_START "Start" 		/** String para identificar comandos: Start#OK!			 **/

//-------- Equivlencia numérica dos comandos do cliente -----------//

#define C_C_START 0 			/** Equivalente numérico da string C_START		**/
#define C_C_COM 1 				/** Equivalente numérico da string C_COMTEST	**/
#define C_C_SET 3 				/** Equivalente numérico da string C_SETMAX		**/
#define C_C_GET 4 				/** Equivalente numérico da string C_GETLV		**/
#define C_C_CLOSE 5 			/** Equivalente numérico da string C_CLOSE		**/
#define C_C_OPEN 6 				/** Equivalente numérico da string C_OPEN		**/

//-------- Equivlencia numérica dos comandos do servidor -----------//

#define C_S_START 10 			/** Equivalente numérico da string S_START		**/
#define C_S_COM 11	   			/** Equivalente numérico da string S_COMTEST	**/
#define C_S_ERRO -1 			/** Equivalente numérico da string S_ERRO		**/
#define C_S_SET 13  			/** Equivalente numérico da string S_SETMAX	 	**/
#define C_S_GET 14 				/** Equivalente numérico da string S_GETLV		**/
#define C_S_CLOSE 15 			/** Equivalente numérico da string S_CLOSE		**/
#define C_S_OPEN 16 			/** Equivalente numérico da string S_OPEN		**/

//----------------- Outros elementos do protocolo -----------------//

#define ENDMSG "!"			  	/** String do fim de mesnsagem do protocolo		**/
#define TK "#"					/** String do separador de mensagem do protocolo**/
#define OK "OK"					/** String definir o status OK					**/

/**
*@brief Contêm as informações de uma mensagem recebida.
*
**/
typedef struct TPMENSAGEM 		
{
	int valor;      // valor da mensagem
	int sequencia;  // numero de sequencia
	int comando;	// comando 
} TPMENSAGEM;


//===================== Cabeçalhos de Funções =====================//

TPMENSAGEM analisarComando(char *mensagem, int is_serv);
void obterInfo(TPMENSAGEM *saida,TPMENSAGEM msg);
 
//#################################################################//
//########################   FUNÇÕES   ############################//
//#################################################################//

/**
 * @brief Verifica integridade da mensagem e retorna o comando/dados relevantes.
 *  Retorna o comando que deve ser eviando pela ponta da comunição que recebeu o 
 *  comando. Exemplos: 
 *  CASO 1:
 *  Cliente -> OpenValve#123#999!
 *  Resposta: retorno : TPMENSAGEM =  int valor = 999;
 *								  int sequencia =123;  
 *									  int comando = 16;	-> servidor deve abrir a válvula
 *  CASO 2:
 *  Servidor -> Open#123!
 *  Resposta: retorno : TPMENSAGEM =  int valor = -1;
 *								      int sequencia =123;  
 *									  int comando = 6;	-> cliente recebeu um ACK
 * 
 * @param mensagem Endereço que contem a messagem (char array)
 * @param is_serv  Define em qual ponta da comunição a função é chamada
 * @return TPMENSAGEM que foi detectado com base na mensagem recebida.
 */
TPMENSAGEM analisarComando(char *mensagem, int is_serv)
{
    // Analisar se a mensagem é válida
	TPMENSAGEM retorno;
	retorno.comando = C_S_ERRO;
	retorno.sequencia = C_S_ERRO;
	retorno.valor= C_S_ERRO;
    int len = strlen(mensagem);
    int opc = 0;
	char *tk; // token
    char *resto = mensagem;
    int numtoken = 0;
	int i =0;
	if (mensagem[len-1] != ENDMSG[0]){//verifica se é o fim de uma msg
        return retorno;
    }
    for(i=0; i<len;i++){ // verificando numero de tokens
		if(mensagem[i]=='#')
			numtoken++;
	}
	if (numtoken >2)return retorno; // se tem muitos  # sai
	// Se  chegar até aqui: tem ! no fim e tem 0, 1 ou 2 tokens
	if (is_serv){
		switch (numtoken){
			case 0: // numero de tokens 
				tk=strtok_r(resto,ENDMSG,&resto); 
				if (strcmp(tk, C_GETLV) == 0){
					retorno.comando = C_S_GET;
				
				}
				else if (strcmp(tk, C_START) == 0){
					retorno.comando =  C_S_START;
				}
				else if (strcmp(tk, C_COMTEST) == 0){
					retorno.comando =  C_S_COM; 
				}
				else{
					return retorno;
				}
			break;
			case 1: // numero de tokens 
				tk = strtok_r(resto, TK, &resto); // pega comando
				if (strcmp(tk, C_SETMAX) == 0){
					//agora tenho que quebrar denovo a string
					tk = strtok_r(resto, TK, &resto); // pega num sequencia
					//pensar o que fazer aqui 
					retorno.sequencia = atoi(tk); // capturei numero de sequencia
					// verificar se podem ocorrer coisas do tipo 001 010L0 e por ai vai 
					//possível solução: utilizar um for
					//if (strcmp(tk,itoa(retorno.sequencia))){}
					retorno.comando = C_S_SET;
				}
				else{
					return retorno;
				}
			break;
			case 2:// numero de tokens 
				tk = strtok_r(resto, TK, &resto); // pega comando
				if (strcmp(tk, C_CLOSE) == 0) retorno.comando = C_S_CLOSE;
				else if (strcmp(tk, C_OPEN) == 0) retorno.comando = C_S_OPEN;
				else {
					return retorno;
				}
			
				//agora tenho que quebrar denovo a string
				tk = strtok_r(resto, TK, &resto); // pega num sequencia
				//pensar o que fazer aqui 
				retorno.sequencia = atoi(tk); // capturei numero de sequencia
				// verificar se podem ocorrer coisas do tipo 001 010L0 e por ai vai 
				//possível solução: utilizar um for
				//if (strcmp(tk,itoa(retorno.sequencia))){}
				tk = strtok_r(resto,ENDMSG,&resto);
				retorno.valor  = atoi(tk); // capturei valor do comando
				if (retorno.valor >= 0 && retorno.sequencia >=0){
					#ifdef DEBUG
					printf("Sequencia :%d \t\tValor :%d\n",retorno.sequencia,retorno.valor);
					#endif
				}
				else{
					retorno.comando =C_S_ERRO;
				}
				return retorno;
			break;
			default:
				return retorno;
		}
	}
	else { // é o cliente 
		if(numtoken !=1) return retorno;
		else {
			
			tk = strtok_r(resto, TK, &resto); // pega comando
			#ifdef DEBUG
				printf("Comandos CLiente\n");
				printf("tk= %s\n",tk);
			#endif	
			//
			if(strcmp(tk,S_COMTEST) == 0 && strcmp(strtok_r(resto, ENDMSG, &resto),OK)==0){ 
				retorno.comando = C_C_COM;
			}
			else if (strcmp(tk,S_START)==0 && strcmp(strtok_r(resto, ENDMSG, &resto),OK)==0) {
				retorno.comando = C_C_START;
			}
			else if (strcmp(tk,S_OPEN)==0) {
				tk = strtok_r(resto, ENDMSG, &resto);
				retorno.sequencia = atoi(tk);
				if (retorno.sequencia>0){
					retorno.comando = C_C_OPEN;
				}
				else {
					retorno.comando =C_S_ERRO;
				}
			}
			else if (strcmp(tk,S_CLOSE)==0) {
				tk = strtok_r(resto, ENDMSG, &resto);
				retorno.sequencia = atoi(tk);
				if (retorno.sequencia>0){
					retorno.comando= C_C_CLOSE;
				}
				else {
					retorno.comando =C_S_ERRO;
				}
			}
			else if (strcmp(tk,S_GETLV)==0) {
				tk = strtok_r(resto, ENDMSG, &resto);
				retorno.valor = atoi(tk);
				if (retorno.valor>0){  // tem um range colocar aqui
					retorno.comando = C_C_GET;
				}
				else {
					retorno.comando =C_S_ERRO;
				}
			}
			else if (strcmp(tk,S_SETMAX)==0) { // existe um range ??
				tk = strtok_r(resto, ENDMSG, &resto);
				retorno.valor = atoi(tk);
				if (retorno.valor>0){
					retorno.comando = C_C_SET;
				}
				else {
					retorno.comando =C_S_ERRO;
				}
			}
			else{
				retorno.comando =C_S_ERRO;
				#ifdef DEBUG
				printf("CLIENTE:\tSequencia :%d \t\tValor :%d\n",retorno.sequencia,retorno.valor);
				#endif
			}	
			return retorno;

		}
	}
    return retorno; // retorno padrão
}

/**
*@brief Função para obter a informações de uma mensagem e escrever em outro local
*
*@param saida Endereço de saída
*@param msg Mensagem de entrada
**/
void obterInfo(TPMENSAGEM *saida,TPMENSAGEM msg){
  	saida->valor =  msg.valor;
  	saida->sequencia =  msg.sequencia;
  	saida->comando =  msg.comando;
}

#define protocolo
#endif
