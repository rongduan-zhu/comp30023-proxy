#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

#define BUFFER_SIZE 256
// #define PORT_NUMBER 80

int main(int argc, char const *argv[])
{
    if (argc != 2) {
        fprintf(stderr,"Incorrect number of arguments.\nUsage: %s port_number\n", argv[0]);
        exit(1);
    }

    int listen_sockfd = 0, connect_sockfd = 0, cli_addr_length;
    struct sockaddr_in serv_addr, cli_addr; 

    char to_client_buffer[BUFFER_SIZE];

    // Create a socket addr family: IPv4, type: steam (IPv4), protocol: 0
    listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(to_client_buffer, '0', sizeof(to_client_buffer)); 

    serv_addr.sin_family = AF_INET;
    // htonl converts unsigned int hostlong from host byte order to network byte order
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1])); 

    // bind the server to the port
    if (bind(listen_sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "ERROR on binding\n");
        exit(1);
    } 

    // start listening on the port
    listen(listen_sockfd, 10); 

    cli_addr_length = sizeof(cli_addr);

    while(1)
    {
        // accepts from client
        connect_sockfd = accept(listen_sockfd, (struct sockaddr*) &cli_addr, &cli_addr_length); 

        if (connect_sockfd < 0) {
            fprintf(stderr, "ERROR on accept\n");
            exit(1);
        } 

        memcpy(to_client_buffer, "You received me bradda!\n", 25);

        write(connect_sockfd, to_client_buffer, strlen(to_client_buffer)); 
        // close(connect_sockfd);
        sleep(1);
     }

    return 0;
}
