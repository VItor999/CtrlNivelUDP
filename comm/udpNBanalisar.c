//----- Include files ---------------------------------------------------------
#include <stdio.h>  // Needed for printf()
#include <string.h> // Needed for memcpy() and strcpy()

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>  // Needed for sockets stuff
#include <netinet/in.h> // Needed for sockets stuff
#include <sys/socket.h> // Needed for sockets stuff
#include <arpa/inet.h>  // Needed for sockets stuff
#include <fcntl.h>      // Needed for sockets stuff
#include <netdb.h>      // Needed for sockets stuff

//----- Defines ---------------------------------------------------------------
#define PORT_NUM 1050       // Port number used
#define IP_ADDR "127.0.0.1" // IP address of server1 (*** HARDWIRED ***)

//===== Main program ==========================================================

int main(int argv, char *argc)
{

    int client_s;                   // Client socket descriptor
    unsigned long int noBlock;      // Non-blocking flag
    struct sockaddr_in server_addr; // Server Internet address
    int addr_len;                   // Internet address length
    char out_buf[4096];             // Output buffer for data
    char in_buf[4096];              // Input buffer for data
    int retcode;                    // Return code

    // Create a socket
    client_s = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_s < 0)
    {
        printf("*** ERROR - socket() failed \n");
        exit(-1);
    }

    int flags = fcntl(client_s, F_GETFL, 0);
    fcntl(client_s, F_SETFL, flags | O_NONBLOCK);

    // Fill-in server socket's address information
    server_addr.sin_family = AF_INET;                 // Address family to use
    server_addr.sin_port = htons(atoi(argc[1]));           // Port num to use
    server_addr.sin_addr.s_addr = inet_addr(IP_ADDR); // IP address to use

    // Assign a message to buffer out_buf
    strcpy(out_buf, "Test message from CLIENT to SERVER");

    // Now send the message to server.
    retcode = sendto(client_s, out_buf, (strlen(out_buf) + 1), 0,
                     (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (retcode < 0)
    {
        printf("*** ERROR - sendto() failed \n");
        exit(-1);
    }

    // Wait to receive a message (need to spin loop on the receive)
    addr_len = sizeof(server_addr);
    retcode = 0;
    while (retcode <= 0)
    {

        sleep(1); // Unix sleep for 1 second
        printf("Wake-up from sleep... \n");
        fflush(stdout);

        retcode = recvfrom(client_s, in_buf, sizeof(in_buf), 0,
                           (struct sockaddr *)&server_addr, &addr_len);
    }

    // Output the received message
    printf("Received from server: %s \n", in_buf);

    retcode = close(client_s);
    if (retcode < 0)
    {
        printf("*** ERROR - close() failed \n");
        exit(-1);
    }
}
