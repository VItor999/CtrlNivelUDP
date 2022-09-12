#define DEBUG 1
#include "protocol.h"

int main()
{
    char mensagem[] = "OpenValve#123#90!";
    int len = 30;
    len = strlen(mensagem);
    char memcpy[len];
    strcpy(memcpy, mensagem);
    int r;
    char *tk;
    char *resto = memcpy;
    int numSeq, valor;
    r = analisarComando(memcpy);
#ifdef DEBUG
    printf("valor de retorno func %d\n", r);
    printf("Mensagem %s\t  MEM COPY %s\n", mensagem, memcpy);
    strcpy(memcpy, mensagem);
#endif
    if (r < 10) // comandos de saída AZUIS NO PDF
    {
        switch (r)
        {
        case C_O_CLOSE:
        case C_O_OPEN:
            // TODO fazer tratamento de erro para o número de sequencia e valor
            tk = strtok_r(resto, TK, &resto);           // elimina a primeira parte
            numSeq = atoi(strtok_r(resto, TK, &resto)); // captura o número de sequencia
            valor = atoi(strtok_r(resto, TK, &resto));  // captura o valor
#ifdef DEBUG
            printf("%s \tNumSeq %d, valor %d\n", tk, numSeq, valor);
#endif
            break;
        case C_O_SET:
            tk = strtok_r(resto, TK, &resto);          // elimina a primeira parte
            valor = atoi(strtok_r(resto, TK, &resto)); // captura o valor
#ifdef DEBUG
            printf("%s \tvalor %d\n", tk, valor);
#endif
            break;
        case C_O_GET:
// DO STUFF
#ifdef DEBUG
            printf("%s \n", mensagem);
#endif
            break;
        case C_O_START:
// DO STUFF
#ifdef DEBUG
            printf("%s \n", mensagem);
#endif
            break;
        case C_O_COM:
// DO STUFF
#ifdef DEBUG
            printf("%s \n", mensagem);
#endif
            break;
        default:
            break;
        }
    }
    else
    {
        switch (r)
        {
        case C_I_CLOSE:
        case C_I_OPEN:
            // TODO fazer tratamento de erro para o número de sequencia e valor
            tk = strtok_r(resto, TK, &resto);           // elimina a primeira parte
            numSeq = atoi(strtok_r(resto, TK, &resto)); // captura o número de sequencia
            break;
        case C_I_SET:
        case C_I_GET:
            tk = strtok_r(resto, TK, &resto);          // elimina a primeira parte
            valor = atoi(strtok_r(resto, TK, &resto)); // captura o valor
            printf("Mensagem %s\tValor", mensagem);
            break;
        case C_I_START:
// DO STUFF
#ifdef DEBUG
            printf("%s \n", mensagem);
#endif
            break;
        case C_I_COM:
#ifdef DEBUG
            printf("%s \n", mensagem);
#endif
            // DO STUFF
            break;
        case C_I_ERRO:
#ifdef DEBUG
            printf("%s \n", mensagem);
#endif
            // DO STUFF
            break;
        default:
            break;
        }
    }
#ifdef DEBUG
    printf("MENSAGEMN TA OK ??? %s\n", mensagem);
#endif
    return 0;
}
