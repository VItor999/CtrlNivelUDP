// includes

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef protocolo
// Defines do Protocolo próprio
// O_XXXX output qualquer comando DE SAIDA (AZUL NO PDF)
#define O_OPEN "OpenValve"
#define O_CLOSE "CloseValve"
#define O_GETLV "GetLevel"
#define O_SETMAX "SetMax"
#define O_COMTEST "CommTest"
#define O_START "Start"

// I_XXXX input com qualquer comando DE RETORNO (VERDE NO PDF)
#define I_OPEN "Open"
#define I_CLOSE "Close"
#define I_GETLV "Level"
#define I_SETMAX "Max"
#define I_ERRO "Err"
#define I_COMTEST "Comm"
#define I_START "Start"

#define C_O_START 0
#define C_O_COM 1
#define C_O_SET 3
#define C_O_GET 4
#define C_O_CLOSE 5
#define C_O_OPEN 6

#define C_I_START 10
#define C_I_COM 11
#define C_I_ERRO 2
#define C_I_SET 13
#define C_I_GET 14
#define C_I_CLOSE 15
#define C_I_OPEN 16

// Terminadores e Token
#define ENDMSG "!"
#define TK "#"

// Cabeçalhos
int analisarComando(char *mensagem,int isServ);
int descComando(char *tipo,int isServ);

// Verifica integridade da mensagem
// Final e inicio correto, não trata conteúdo
int analisarComando(char *mensagem,int isServ)
{
    // Analisar se a mensagem é válida
    int len = strlen(mensagem);
    char *tk; // token
    char *resto = mensagem;
    int opc = 0;
    if (mensagem[len - 1] != ENDMSG[0])
    {
        return C_I_ERRO;
    }
    else
    {
        tk = strtok_r(resto, TK, &resto);
        opc = descComando(tk,isServ);
        return opc;
    }
}

// Descobre Qual é o comando
int descComando(char *tipo, int isServ)
{
    // Extremamente hardcoded, buscar otimizar depois
    //  Sequencia de ifs em ordem de relevancia
    if (isServ)
    {
        // COMANDOS COM 0 => COMANDOS EM AZUL NO PDF (EMITIDOS) PELO ELEMENTO PENSANTE DO SISTEMA
        if (strcmp(tipo, O_OPEN) == 0)
            return C_O_OPEN;
        if (strcmp(tipo, O_CLOSE) == 0)
            return C_O_CLOSE;
        if (strcmp(tipo, O_GETLV) == 0)
            return C_O_GET;
        if (strcmp(tipo, O_SETMAX) == 0)
            return C_O_SET;
        if (strcmp(tipo, O_START) == 0)
            return C_O_START;
        if (strcmp(tipo, O_COMTEST) == 0)
            return C_O_COM;
    }
    else
    {
        // COMANDOS COM I => COMANDOS EM VERDE DO PDF
        if (strcmp(tipo, I_OPEN) == 0)
            return C_I_OPEN;
        if (strcmp(tipo, I_CLOSE) == 0)
            return C_I_CLOSE;
        if (strcmp(tipo, I_GETLV) == 0)
            return C_I_GET;
        if (strcmp(tipo, I_SETMAX) == 0)
            return C_I_SET;
        if (strcmp(tipo, I_START) == 0)
            return C_I_START;
        if (strcmp(tipo, I_COMTEST) == 0)
            return C_I_COM;
    }
    return C_I_ERRO; // retorno padrão
}
#define protocolo
#endif
