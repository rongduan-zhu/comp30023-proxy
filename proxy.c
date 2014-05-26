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

#define BUFFER_SIZE 2048
#define HOST_PORT 80

// int get_message_from_host(int my_sockfd, const struct socketaddr_in* host);
char *build_query(char *);

int main(int argc, char const *argv[])
{
    if (argc != 2) {
        fprintf(stderr,"Incorrect number of arguments.\nUsage: %s port_number\n", argv[0]);
        exit(1);
    }

    /* Setup process */

    int my_sockfd = 0, 
        client_sockfd = 0, 
        host_sockfd = 0, 
        cli_addr_length;
    struct sockaddr_in proxy_addr, cli_addr, host_addr; 
    struct hostent *host;

    char to_client_buffer[BUFFER_SIZE];

    // Create a socket addr family: IPv4, type: steam (IPv4), protocol: 0
    my_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&proxy_addr, 0, sizeof(proxy_addr));
    memset(to_client_buffer, 0, sizeof(to_client_buffer));
    memset(&host_addr, 0, sizeof(host_addr)); 

    // Sets up server
    proxy_addr.sin_family = AF_INET;
    // htonl converts unsigned int hostlong from host byte order to network byte order
    proxy_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    proxy_addr.sin_port = htons(atoi(argv[1])); 


    // bind the server to the port
    if (bind(my_sockfd, (struct sockaddr*) &proxy_addr, sizeof(proxy_addr)) < 0) {
        fprintf(stderr, "ERROR on binding\n");
        exit(1);
    } 

    // start listening on the port
    listen(my_sockfd, 10); 

    cli_addr_length = sizeof(cli_addr);

    /* End of setup */

    client_sockfd = accept(my_sockfd, (struct sockaddr*) &cli_addr, &cli_addr_length); 

    // accepts from client
    if (client_sockfd < 0) {
        fprintf(stderr, "ERROR on accept\n");
        exit(1);
    } 

    // read host from client
    int bread = 0,
        bwrite = 0;
    char hostname[BUFFER_SIZE];
    char *query;
    
    if (0 == read(client_sockfd, hostname, BUFFER_SIZE)) {
        fprintf(stderr, "No host given\n");
        close(client_sockfd);
        exit(EXIT_FAILURE);
    }

    // setup host
    host = gethostbyname(hostname);

    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(HOST_PORT);
    bcopy((char *)host->h_addr, 
          (char *)&host_addr.sin_addr.s_addr,
          host->h_length);
    if (host == NULL) {
        fprintf(stderr,"No such host\n");
        exit(0);
    }

    // build get query
    query = build_query(hostname);

    // connect to host
    host_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(host_sockfd, (struct sockaddr *)&host_addr, sizeof(struct sockaddr)) < 0) 
    {
         fprintf(stderr, "Can't connect to host\n.");
         exit(EXIT_FAILURE);
    }

    fprintf(stderr, "Query: %s\n", query);
    if (0 == (bwrite = write(host_sockfd, query, strlen(query)))) {
        fprintf(stderr, "Can't write to host\n.");
        exit(EXIT_FAILURE);
    }

    char response[BUFFER_SIZE];
    // Keep on reading response from server
    while(0 < (bread = read(host_sockfd, response, BUFFER_SIZE))) {
        // write response back to client
        write(client_sockfd, response, bread);
        memset(response, 0, BUFFER_SIZE);
    }

    close(host_sockfd);
    close(client_sockfd);
    close(my_sockfd);
    // memcpy(to_client_buffer, "You received me bradda!\n", 25);

    // write(client_sockfd, to_client_buffer, strlen(to_client_buffer)); 
    // close(client_sockfd);

    return 0;
}

char *build_query(char *host) {
    char *get = "GET / HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n";
    char *query;
    query = (char *) malloc(strlen(host)
                            + strlen(get)
                            - 3);
    sprintf(query, get, host);
    return query;
}
