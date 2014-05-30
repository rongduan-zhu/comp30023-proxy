/*
    Multithreaded simplified proxy program
    Author:           Maxim Lobanov & Rongduan Zhu
    Last Modified:    30/05/2014
 */

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
#include <time.h>

/* Buffer size used for reading response from server and requests from client */
#define BUFFER_SIZE 2048
/* Buffer size used for sending a error message back to client */
#define SMALL_BUFFER_SIZE 256
#define HOST_PORT 80
#define MAX_CONNECTIONS 50
/* 29 because the "GET http:/// HTTP/1.0\r\n\r\n" is 29 characters*/
#define MIN_CLIENT_CHUNK_BYTE 29

/* Request handling function used to handle request from client */
void *request_handler(void *);

/* Builds a get request from a hostname (should be deleted as new client creates
   the request (Not used because new version)
*/
char *build_query(char *);

/* Get host from query */
char *get_host_from_query(char *);

/* Print log for a request */
void print_log(char *ip, int port_number, int bytes_sent, char *requested_hostname);

/* mutex lock */
pthread_mutex_t lock;

/* input struct for thread */
typedef struct {
    int client_sockfd;
    struct sockaddr_in cli_addr;
} thread_args;

int main(int argc, char const *argv[])
{
    if (argc != 2) {
        fprintf(stderr,"Incorrect number of arguments.\nUsage: %s port_number\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // ignoring broken pipe signal
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        fprintf(stderr, "Can't handle broken pipe issue\n");
    }

    // initialize mutex lock
    if (pthread_mutex_init(&lock, NULL) != 0) {
        fprintf(stderr, "Can't initialize mutex lock\n");
        exit(EXIT_FAILURE);
    }

    /* Setup process */
    int proxy_sockfd = 0,
        client_sockfd = 0,
        cli_addr_length;

    struct sockaddr_in proxy_addr,
                       cli_addr;

    // Create a socket addr family: IPv4, type: steam (IPv4), protocol: 0
    proxy_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&proxy_addr, 0, sizeof(proxy_addr));

    // setting variable used for accept
    cli_addr_length = sizeof(cli_addr);

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
    listen(proxy_sockfd, MAX_CONNECTIONS);

    /* End of setup for proxy, now listening */

    // starts accepting requests
    while (1) {
        pthread_t thread;
        if (0 > (client_sockfd = accept(proxy_sockfd, (struct sockaddr*) &cli_addr, &cli_addr_length))) {
            fprintf(stderr, "Error on accept with error %d\n", errno);
            exit(EXIT_FAILURE);
        }

        thread_args *thread_arg;
        thread_arg = (thread_args *) malloc(sizeof(thread_args));
        if (thread_arg == NULL) {
            fprintf(stderr, "Oh no! Not enough memory to create new thread\nSkipping request\n");
            continue;
        }
        thread_arg->client_sockfd = client_sockfd;
        thread_arg->cli_addr = cli_addr;

        // on receiving a request, spawn new thread
        int err = pthread_create(&thread, NULL, request_handler, (void*) thread_arg);
        if (err) {
            fprintf(stderr, "Unable to create thread with error message %d\n", err);
            sleep(5);
            continue;
        }

        // detach the thread
        if (pthread_detach(thread)) {
            fprintf(stderr, "Unable to detach thread\n");
            sleep(5);
            continue;
        }
    }

    // close socket and destroy mutex lock
    close(proxy_sockfd);
    pthread_mutex_destroy(&lock);

    fprintf(stderr, "Completed all requests\n");
    return 0;
}

void *request_handler(void *thread_arg) {
    // initializes socket file descriptors
    thread_args arg = *((thread_args *) thread_arg);
    int client_sockfd = arg.client_sockfd,
        host_sockfd = 0,
        bread = 0,
        bwrite = 0,
        total_bread = 0,
        total_bwrite = 0;

    char response[BUFFER_SIZE],
         query_buffer[BUFFER_SIZE],
         temp_buffer[BUFFER_SIZE],
         *hostname,
         *query;

    // address structs
    struct sockaddr_in cli_addr, host_addr;
    // stores address about host returned by gethostbyname
    struct hostent *host;
    cli_addr = arg.cli_addr;

    // clear memory for unsed thread arguments
    free((thread_args *) thread_arg);

    // declaring i/o buffers
    char to_client_buffer[BUFFER_SIZE];

    // initialize buffers and address structs
    memset(to_client_buffer, 0, sizeof(to_client_buffer));
    memset(&host_addr, 0, sizeof(host_addr));
    memset(query_buffer, '\0', BUFFER_SIZE);
    memset(temp_buffer, '\0', BUFFER_SIZE);

    // accepting connection from clien = sizeof(cli_addr);

    // read message/request sent from client, this gets blocked if client
    // don't sends a complete http request
    int end_of_request_flag = 0;
    while (0 < (bread = read(client_sockfd, temp_buffer, BUFFER_SIZE))) {
        // close connection if buffer overflow
        if (bread + total_bread > BUFFER_SIZE) {
            char error_message[SMALL_BUFFER_SIZE] = "Query too big\n";
            fprintf(stderr, "Buffer overflow, terminating request\n");
            write(client_sockfd, error_message, strlen(error_message));

            // close descriptors
            close(client_sockfd);
            return NULL;
        }

        // appends temp_buffer to query_buffer
        strncat(query_buffer, temp_buffer, bread);
        total_bread += bread;

        if (total_bread > MIN_CLIENT_CHUNK_BYTE) {
            // checks if request is done, if it is, stop reading
            if (query_buffer[total_bread - 1] == '\n'
                && query_buffer[total_bread - 2] == '\r'
                && query_buffer[total_bread - 3] == '\n'
                && query_buffer[total_bread - 4] == '\r') {
                end_of_request_flag = 1;
                break;
            }
        }

        // checks if client has finished request by reading the last 4 bytes
        // and see if it matches \r\n\r\n
        memset(temp_buffer, '\0', BUFFER_SIZE);
    }

    if (end_of_request_flag != 1 || bread < 0) {
        fprintf(stderr, "Invalid request or client terminated early.\n");

        // close sockets
        close(client_sockfd);

        return NULL;
    }

    query = query_buffer;
    // get host name from query
    hostname = get_host_from_query(query);
    if (hostname == NULL) {
        // close sockets
        close(client_sockfd);

        return NULL;
    }

    // setup host
    host = gethostbyname(hostname);
    if (host == NULL) {
        fprintf(stderr,"No such host\n");

        // close sockets
        close(client_sockfd);

        // free memory
        free(hostname);
        return NULL;
    }
    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(HOST_PORT);

    memcpy((char *)&host_addr.sin_addr.s_addr,
           (char *)host->h_addr,
           host->h_length);

    // connect to host
    host_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(host_sockfd, (struct sockaddr *)&host_addr, sizeof(struct sockaddr)) < 0) {
        fprintf(stderr, "Can't connect to host.\n");

        // close sockets
        close(client_sockfd);

        // free memory
        free(hostname);

        return NULL;
    }

    // forward request onto server
    if (0 == (bwrite = write(host_sockfd, query, strlen(query)))) {
        fprintf(stderr, "Can't write to host\n.");

        // close sockets
        close(host_sockfd);
        close(client_sockfd);

        // free memory
        free(hostname);
        return NULL;
    }

    // keep on reading response from server
    total_bwrite = 0;
    while(0 < (bread = read(host_sockfd, response, BUFFER_SIZE))) {
        // write response back to client
        if(0 > (bwrite = write(client_sockfd, response, bread))) {
            fprintf(stderr, "Client terminated early.\n");
            // return NULL;
            break;
        }
        memset(response, 0, BUFFER_SIZE);
        total_bwrite += bwrite;
    }


    char client_addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(host_addr.sin_addr), client_addr, INET_ADDRSTRLEN);

    print_log(client_addr, cli_addr.sin_port, total_bwrite, hostname);

    // close sockets
    close(host_sockfd);
    close(client_sockfd);

    // free memory
    free(hostname);

    return NULL;
}

char *build_query(char *host) {
    char *get = "GET / HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n";
    char *query;
    query = (char *) malloc(strlen(host)
                            + strlen(get)
                            - 3);
    // not using "get" as second argument as compiler gives a warning, and we might
    // get mark deduction. After googling, the warning is completely normal, and
    // it can be safely ignored, but I don't wanna risk it
    if (query == NULL) {
        fprintf(stderr, "Oh no :O, not enough memory!\nClosing Connection\n");
        return query;
    }
    sprintf(query, "GET / HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", host);
    return query;
}

char *get_host_from_query(char *query) {
    // start is at 4 because get+space, its static
    int start = 11,
        end = 0;
    char *hostname = (char *) malloc(BUFFER_SIZE);
    if (hostname == NULL) {
        fprintf(stderr, "Oh no :O, not enough memory!\nClosing Connection\n");
        return hostname;
    }
    for (int i = start; i < (int) strlen(query); ++i) {
        if (query[i] == ' ') {
            end = i;
            break;
        }
    }
    // append end of string
    memcpy(hostname, &query[start], end - start);
    hostname[end - start - 1] = '\0';
    return hostname;
}

void print_log(char *ip, int port_number, int bytes_sent, char *requested_hostname) {
    FILE *fp;
    time_t current_time;
    char *time_string;

    // get current time
    current_time = time(NULL);

    if (NULL == (time_string = ctime(&current_time))) {
        fprintf(stderr, "Unable to convert current time.\n");
        time_string = "N/A";
    }

    // remove new line appended by ctime
    time_string[strlen(time_string) - 1] = '\0';

    // apply mutex lock
    pthread_mutex_lock(&lock);

    fp = fopen("proxy.log", "a");

    fprintf(fp, "%s,%s,%d,%d,%s\n", time_string, ip, port_number, bytes_sent, requested_hostname);
    fprintf(stderr, "%s, %s, %d, %d, %s\n", time_string, ip, port_number, bytes_sent, requested_hostname);

    fclose(fp);

    // unlock mutex
    pthread_mutex_unlock(&lock);
}
