#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef protocolo
#define DEBUG 1
// Defines do Protocolo próprio escrito do ponto de vista do cliente (COMANDOS AZUIS)
// O_XXXX output qualquer comando DE SAIDA (AZUL NO PDF)
// comando#XX#YYY!
//		  12345678												  TS Tdoido
#define C_OPEN "OpenValve" 		// OpenValve#123#999! 	
#define C_CLOSE "CloseValve"	// CloseValve#13#999!  		  
#define C_GETLV "GetLevel"		// GetLevel!		 	Resolvido OK
#define C_SETMAX "SetMax" 		// SetMax#123!		 	Resolvido OK
#define C_COMTEST "CommTest" 	// CommTest!		 	Resolvido OK
#define C_START "Start"			// Start!  			 	Resolvido OK

// I_XXXX input com qualquer comando DE RETORNO (VERDE NO PDF)
#define S_OPEN "Open"     		// Open#123!
#define S_CLOSE "Close"  		// Close#123!
#define S_GETLV "Level" 		// Level#999!
#define S_SETMAX "Max" 			// Max#999!
#define S_ERRO "Err" 			// Err!   			   Resolvido OK
#define S_COMTEST "Comm" 		// Comm#Ok!        
#define S_START "Start" 		// Start#Ok!

#define C_C_START 0 			
#define C_C_COM 1 				
#define C_C_SET 3 				
#define C_C_GET 4 				
#define C_C_CLOSE 5 			
#define C_C_OPEN 6 				

#define C_S_START 10 			
#define C_S_COM 11	   			
#define C_S_ERRO -1 			
#define C_S_SET 13  			
#define C_S_GET 14 				
#define C_S_CLOSE 15 			
#define C_S_OPEN 16 			

// Terminadores e Token
#define ENDMSG "!"
#define TK "#"
//
//typedef struct TPMENSAGEM // o que tu acha de retornarmos isso como resposta do analisa comando???
//{
//	int valor;
//	int sequencia;
//	int satus;
//	int comando;
//} TPMENSAGEM;


// Cabeçalhos
int analisarComando(char *mensagem, int is_serv);

// Verifica integridade da mensagem
// Final e inicio correto, não trata conteúdo
int analisarComando(char *mensagem, int is_serv)
{
    // Analisar se a mensagem é válida
    int len = strlen(mensagem);
    int opc = 0;
	char *tk; // token
    char *resto = mensagem;
    int numtoken = 0;
    int numSequencia=0;   // preciso jogar isso pra fora
	int valorComando=0;  // preciso jogar isso pra fora
	int flagComando=-1; // preciso jogar isso para fora
	int i =0;
	if (mensagem[len - 1] != ENDMSG[0]){//verifica se é o fim de uma msg
        return C_S_ERRO;
    }
    for(i=0; i<len;i++){ // verificando numero de tokens
		if(mensagem[i]=='#')
			numtoken++;
	}
	if (numtoken >2)return C_S_ERRO; // se tem muitos  # sai
	// Se  chegar até aqui: tem ! no fim e tem 0, 1 ou 2 tokens
	if (is_serv){
		switch (numtoken){
			case 0: // numero de tokens 
				tk=strtok_r(resto,ENDMSG,&resto); 
				if (strcmp(tk, C_GETLV) == 0){
					return C_S_GET;
				}
				else if (strcmp(tk, C_START) == 0){
					return C_S_START;
				}
				else if (strcmp(tk, C_COMTEST) == 0){
					return C_S_COM; 
				}
				else{
					return C_S_ERRO;
				}
			break;
			case 1: // numero de tokens 
				tk = strtok_r(resto, TK, &resto); // pega comando
				if (strcmp(tk, C_SETMAX) == 0){
					//agora tenho que quebrar denovo a string
					tk = strtok_r(resto, TK, &resto); // pega num sequencia
					//pensar o que fazer aqui 
					numSequencia = atoi(tk); // capturei numero de sequencia
					// verificar se podem ocorrer coisas do tipo 001 010L0 e por ai vai 
					//possível solução: utilizar um for
					//if (strcmp(tk,itoa(numSequencia))){}
					return C_S_SET;
				}
				else{
					return C_S_ERRO;
				}
			break;
			case 2:// numero de tokens 
				tk = strtok_r(resto, TK, &resto); // pega comando
				if (strcmp(tk, C_CLOSE) == 0) flagComando =C_S_CLOSE;
				else if (strcmp(tk, C_OPEN) == 0) flagComando =C_S_OPEN;
				else return C_S_ERRO;
			
				//agora tenho que quebrar denovo a string
				tk = strtok_r(resto, TK, &resto); // pega num sequencia
				//pensar o que fazer aqui 
				numSequencia = atoi(tk); // capturei numero de sequencia
				// verificar se podem ocorrer coisas do tipo 001 010L0 e por ai vai 
				//possível solução: utilizar um for
				//if (strcmp(tk,itoa(numSequencia))){}
				tk = strtok_r(resto,ENDMSG,&resto);
				valorComando  = atoi(tk); // capturei valor do comando
				if (valorComando >= 0 && numSequencia >=0){
					#ifdef DEBUG
					printf("Sequencia :%d \t\tValor :%d\n",numSequencia,valorComando);
					#endif
					return flagComando;
				}
				else{
					return C_S_ERRO;
				}
			break;
			default:
				return C_S_ERRO;
		}
	}
	else { // é o cliente 
		if(numtoken !=1) return C_S_ERRO;
		else {
			
			tk = strtok_r(resto, TK, &resto); // pega comando
			#ifdef DEBUG
				printf("Comandos CLiente\n");
				printf("tk= %s\n",tk);
			#endif	
			flagComando = C_S_ERRO;
			//
			if(strcmp(tk,S_COMTEST) == 0 && strcmp(strtok_r(resto, ENDMSG, &resto),"OK")==0){ 
				flagComando = C_C_COM;
			}
			else if (strcmp(tk,S_START)==0 && strcmp(strtok_r(resto, ENDMSG, &resto),"OK")==0) {
				flagComando = C_C_START;
			}
			else if (strcmp(tk,S_OPEN)==0) {
				tk = strtok_r(resto, ENDMSG, &resto);
				numSequencia = atoi(tk);
				if (numSequencia>0){
					flagComando = C_C_OPEN;
				}
				else {
					flagComando =C_S_ERRO;
				}
			}
			else if (strcmp(tk,S_CLOSE)==0) {
				tk = strtok_r(resto, ENDMSG, &resto);
				numSequencia = atoi(tk);
				if (numSequencia>0){
					flagComando = C_C_CLOSE;
				}
				else {
					flagComando =C_S_ERRO;
				}
			}
			else if (strcmp(tk,S_GETLV)==0) {
				tk = strtok_r(resto, ENDMSG, &resto);
				valorComando = atoi(tk);
				if (valorComando>0){  // tem um range colocar aqui
					flagComando = C_C_GET;
				}
				else {
					flagComando =C_S_ERRO;
				}
			}
			else if (strcmp(tk,S_SETMAX)==0) { // existe um range
				tk = strtok_r(resto, ENDMSG, &resto);
				valorComando = atoi(tk);
				if (valorComando>0){
					flagComando = C_C_SET;
				}
				else {
					flagComando =C_S_ERRO;
				}
			}
			else{
				printf("CLIENTE:\tSequencia :%d \t\tValor :%d\n",numSequencia,valorComando);
			}	
			return flagComando;

		}
	}
    return C_S_ERRO; // retorno padrão
}


#define protocolo
#endif
