// Server program
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#define PORT 5000
#define MAXLINE 1024
int max(int x, int y)
{
	if (x > y)
		return x;
	else
		return y;
}
int main()
{
	int listenfd, connfd, udpfd, nready, maxfdp1;
	char buffer[MAXLINE];
	pid_t childpid;
	fd_set rset;
	ssize_t n;
	socklen_t len;
	const int on = 1;
	struct sockaddr_in cliaddr, servaddr;
	char* message = "Hello Client";
	void sig_chld(int);

	/* create listening TCP socket 
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	bzero(&servaddr, sizeof(servaddr));
	*/
    servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(PORT);
    /*
	// binding server addr structure to listenfd
	bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	listen(listenfd, 10);
    //v */

	/* create UDP socket */
    //buffer de tamanho padrão udpfd
	udpfd = socket(AF_INET, SOCK_DGRAM, 0);
	// binding server addr structure to udp sockfd
	bind(udpfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

	// clear the descriptor set
	FD_ZERO(&rset);

	// get maxfd
	maxfdp1 = udpfd+1;//v //max(listenfd, udpfd) + 1;
	for (;;) {

		// set listenfd and udpfd in readset
		//v FD_SET(listenfd, &rset);
        // Setar o descritor de arquivo ou porta
        // UDPFD é o socket ->  fixed size buffer.
		FD_SET(udpfd, &rset);

		// select the ready descriptor
        //Returns: positive count of descriptors ready, 0 on timeout, -1 error
        //maxfdp1: maximum number of descriptors ready.
		nready = select(maxfdp1, &rset, NULL, NULL, NULL);

		//v if tcp socket is readable then handle
		// it by accepting the connection
        /*
		if (FD_ISSET(listenfd, &rset)) {
			len = sizeof(cliaddr);
			connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);
			if ((childpid = fork()) == 0) {
				close(listenfd);
				bzero(buffer, sizeof(buffer));
				printf("Message From TCP client: ");
				read(connfd, buffer, sizeof(buffer));
				puts(buffer);
				write(connfd, (const char*)message, sizeof(buffer));
				close(connfd);
				exit(0);
			}
			close(connfd);
		}*/
		// if udp socket is readable receive the message.
        if (nready>0) {
            // Se select retornou algo 
            if (FD_ISSET(udpfd, &rset)) {
                // socklen_t  -> reserva espaço para o endereço do cliente 
                len = sizeof(cliaddr);
                //limpei o buffer 
                bzero(buffer, sizeof(buffer));
                printf("\nMessage from UDP client: ");
                // preenche buffer com o conteúdo da mensagem e 
                // cliaddr com o endereço de quem mandou a me
                n = recvfrom(udpfd, buffer, sizeof(buffer), 0,
                            (struct sockaddr*)&cliaddr, &len);
                //imprime na tela
                puts(buffer);
                sendto(udpfd, (const char*)message, sizeof(buffer), 0,
                    (struct sockaddr*)&cliaddr, sizeof(cliaddr));
            }
        }
        else{
            printf("Erro %d\n",nready);
        }
	}
}
