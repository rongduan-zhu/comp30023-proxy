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
#include <signal.h>
#include <pthread.h>

#define BUFFER_SIZE 2048
#define HOST_PORT 80
#define MAX_THREADS 12
#define MAX_CONNECTION 10

/* Request handling function used to handle request from client */
void *request_handler(void *);

/* Builds a get request from a hostname (should be deleted as new client creates
   the request
*/
char *build_query(char *);

/* Print log for a request */
void print_log(char *ip, int port_number, int bytes_sent, char *requested_hostname);

int main(int argc, char const *argv[])
{
    if (argc != 2) {
        fprintf(stderr,"Incorrect number of arguments.\nUsage: %s port_number\n", argv[0]);
        exit(1);
    }

    // Ignoring broken pipe signal
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        fprintf(stderr, "Can't handle broken pipe issue\n");
    }

    /* Setup process */
    int proxy_sockfd = 0;

    struct sockaddr_in proxy_addr;

    // Create a socket addr family: IPv4, type: steam (IPv4), protocol: 0
    proxy_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&proxy_addr, 0, sizeof(proxy_addr));

    // Sets up server
    proxy_addr.sin_family = AF_INET;
    // htonl converts unsigned int hostlong from host byte order to network byte order
    proxy_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    proxy_addr.sin_port = htons(atoi(argv[1]));


    // bind the server to the port
    if (bind(proxy_sockfd, (struct sockaddr*) &proxy_addr, sizeof(proxy_addr)) < 0) {
        fprintf(stderr, "Unable to bind to port\n");
        exit(1);
    }

    // start listening on the port
    listen(proxy_sockfd, MAX_CONNECTION);

    /* End of setup for proxy, now listening */

    // creating threads
    pthread_t threads[MAX_THREADS];
    for (int i = 0; i < MAX_THREADS; ++i) {
        int err = pthread_create(&threads[i], NULL, request_handler, (void*) &proxy_sockfd);
        if (err) {
            fprintf(stderr, "Unable to create thread %d with error message %d\n", i, err);
        }
    }

    // joining threads
    for (int i = 0; i < MAX_THREADS; ++i) {
        if (pthread_join(threads[i], NULL)) {
            fprintf(stderr, "Unable to join thread %d\n", i);
        }
    }

    close(proxy_sockfd);

    fprintf(stderr, "Completed all requests\n");
    return 0;
}

void *request_handler(void *sockfd) {
    // initializes socket file descriptors
    int proxy_sockfd = *((int *) sockfd),
        client_sockfd = 0,
        host_sockfd = 0,
        bread = 0,
        bwrite = 0,
        total_bread = 0;
        cli_addr_length;

    char hostname[BUFFER_SIZE],
         response[BUFFER_SIZE],
         *query;

    // address structs
    struct sockaddr_in cli_addr, host_addr;
    // stores address about host returned by gethostbyname
    struct hostent *host;

    // declaring i/o buffers
    char to_client_buffer[BUFFER_SIZE];

    // initialize buffers and address structs
    memset(to_client_buffer, 0, sizeof(to_client_buffer));
    memset(&host_addr, 0, sizeof(host_addr));

    // accepting connection from client
    cli_addr_length = sizeof(cli_addr);
    client_sockfd = accept(proxy_sockfd, (struct sockaddr*) &cli_addr, &cli_addr_length);
    if (client_sockfd < 0) {
        fprintf(stderr, "ERROR on accept\n");
        exit(1);
    }

    // read message/request sent from client
    if (0 == read(client_sockfd, hostname, BUFFER_SIZE)) {
        fprintf(stderr, "No host given\n");
        close(client_sockfd);
        return;
    }

    // setup host
    host = gethostbyname(hostname);
    if (host == NULL) {
        fprintf(stderr,"No such host\n");
        exit(0);
    }
    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(HOST_PORT);

    /*POSSIBLE SOURCE OF BUG*/
    bcopy((char *)host->h_addr,
           (char *)&host_addr.sin_addr.s_addr,
           host->h_length);
    /*POSSIBLE SOURCE OF BUG*/

    // build get query
    query = build_query(hostname);
    fprintf(stderr, "Query: %s\n", query);

    // connect to host
    host_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(host_sockfd, (struct sockaddr *)&host_addr, sizeof(struct sockaddr)) < 0) {
         fprintf(stderr, "Can't connect to host\n.");
         return;
    }

    // forward request onto server
    if (0 == (bwrite = write(host_sockfd, query, strlen(query)))) {
        fprintf(stderr, "Can't write to host\n.");
        return;
    }

    // keep on reading response from server
    while(0 < (bread = read(host_sockfd, response, BUFFER_SIZE))) {
        // write response back to client
        write(client_sockfd, response, bread);
        memset(response, 0, BUFFER_SIZE);
        total_bread += bread;
    }


    char client_addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(host_addr.sin_addr), client_addr, INET_ADDRSTRLEN);
    fprintf(stderr, "%s %d\n", client_addr, cli_addr.sin_port);

    print_log(client_addr, cli_addr.sin_port, total_bread, hostname);

    // close sockets
    close(host_sockfd);
    close(client_sockfd);
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

void print_log(char *ip, int port_number, int bytes_sent, char *requested_hostname) {
    FILE *fp;
    time_t

    fp = fopen("proxy.log", "a");
    fprintf(fp, "%")
}
