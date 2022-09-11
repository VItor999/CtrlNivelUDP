#include "protocol.h"
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <math.h>  
#include "kbhit.h"
#define BUFFER_SIZE 1024

void error(char *msg) // método para imprimir um erro, só passar uma mensagem
{
    perror(msg);
    exit(0); // se colocar exit(1) ele não sai do programa
}

int main(int argc, char *argv[])
{
    // lembrando que: argc = total de argumentos da chamada
    //               *argv = argumentos recebidos (OBS: 0 = chamada do program, 1 - n o resto)
    // Argumentos recebibos sempre em array de char (string)
    // Só preciso passar a porta no servidor
    int sock, length, fromlen, n;
    struct sockaddr_in server;
    struct sockaddr_in from;
    char buffer[BUFFER_SIZE];
    char c = '\0'; // tecla em que irei guardar a saida
    if (argc < 2)  // chamada de programa + porta
    {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(0);
    }
    sock = socket(AF_INET, SOCK_DGRAM, 0); // SOCK_STREAM -> TCP/IP
    fcntl(sock, F_SETFL, O_NONBLOCK);
    if (sock < 0)
    {
        error("Opening socket");
    }
    length = sizeof(server);
    bzero(&server, length); // limpando os valores do server
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(atoi(argv[1])); // converte o valor da porta para o formato necessário
    // Binding
    if (bind(sock, (struct sockaddr *)&server, length) < 0)
    {
        error("binding");
    }
    fromlen = sizeof(struct sockaddr_in);
    // recvfrom recebe algo sendto envia
    while (c != 27) // pressionando esq encerro o server
    {
        // bloco para captura de tecla
        if (kbhit())
        { // LINUX não tem um kbhit() como o windows -> ver arquivo kbhit.h
            c = getchar();
        }
        // de onde vem, onde escrevo, tamanho, argumento padrao, estrutura e tamanho
        n = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&from, &fromlen);
        if (n < 0 && n != -1)
        {
            error("Receber");
        }
        if (n > 0)
        {
            char *tk;
            char *resto = buffer;
            tk = strtok_r(resto, "\n", &resto);
            printf("Received a datagram: %s \n", buffer);
            char mensagem[strlen(buffer)];
            strcpy(mensagem, buffer);
            int r = analisarComando(mensagem); //aqui já sei o que tenho que fazer 
            // passar isso por um switch ? escrever isso como retorno para a main? 
            // colocar minha mensagem em algum lugar ?? area da main ? 
            // Servidor é uma thread -> preciso passar uma serie de elementos para ele 
            // preciso montar meu struct fora daqui e seguir a vida, passa ele para fazer o bindig na thread ???
            // Minha váriavel R será "global" e usarei mutex para lock e etc (uma vez) por ciclo eu atualizo ela ??
            // com base na variavel R sei o que fazer dar uma olhada em teste rapido.c  
            // muitas ideias/ formas de fazer
            // precisamos definir melhor como vamos abordar o problema
            // sem isso eu não sei mt bem como prossseguir 
            char retorno [strlen("Comando")+3]; 
            snprintf(retorno, strlen("Comando")+3, "%d", r);
            n = sendto(sock, retorno, strlen(retorno), 0, (struct sockaddr *)&from, fromlen);
            if (n < 0)
            {
                error("Envio");
            }
            bzero(buffer, BUFFER_SIZE);
        }
    }
    if (c == 27)
    {
        printf("Encerrando SERVIDOR, ESQ pressionado\n");
    }
}
