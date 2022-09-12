
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h> // para non blocking sockets

#define BUFFER_SIZE 1024
void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[BUFFER_SIZE];
    char c = '\0'; // tecla em que irei guardar a saida
    char flagAguardo = 0;
    if (argc < 3)
    {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    if (sockfd < 0)
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");
    printf("Please enter the message: ");
    bzero(buffer, BUFFER_SIZE);
    while (c != 27)
    {
        if (flagAguardo) // estaria aguardando receber a resposta para continuar com uma nova escrita
        {
            printf("continuo rodando\n");
        }
        else // mandar uma  nova mensagem
        {
            fgets(buffer, BUFFER_SIZE - 1, stdin);
              if  ((buffer[0]=='q' || buffer[0]=='Q') && buffer[1] =='\n'){
                c = 27; //ESC
                }
            n = write(sockfd, buffer, strlen(buffer));
            if (n < 0)
                error("ERROR writing to socket");
            flagAguardo = 1;
        }
        bzero(buffer, BUFFER_SIZE);
        n = read(sockfd, buffer, BUFFER_SIZE - 1);
        if (n < 0 && n != -1) // -1 quando não existe nada para ser lido
            error("ERROR reading from socket");
        else if (n != -1)
        {
            printf("LIDO: ");
            printf("%s\n", buffer);
            flagAguardo =0;
            bzero(buffer, BUFFER_SIZE);
        }
    }
    close(sockfd);
    return 0;
}
