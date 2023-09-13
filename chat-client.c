/*
*   chat-client.c
*/

/* Header Files */ 
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <netdb.h>         
#include <pthread.h>
#include <string.h>
#include <strings.h>

/* Constants and Global Vars */ 
#define BUF_SIZE   4096
#define CLIENT_CLOSED 1
#define SERVER_CLOSED 2  
static int conn_fd  = 0;

/* Function Prototypes */ 
static inline void perror_exit(char *cmd);
void *send_routine();
void *recv_routine();
 
/* Main */
int main(int argc, char *argv[])
{
    char *dest_hostname, *dest_port;
    struct addrinfo hints, *res;
    int rc;

    /* Cmd line arguments specify hostname and port. */
    dest_hostname = argv[1];
    dest_port     = argv[2];

    /* Create socket. */
    if ((conn_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror_exit("socket()");
    }

    /* Find IP address of the server. */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if ((rc = getaddrinfo(dest_hostname, dest_port, &hints, &res)) != 0) 
    {
        close(conn_fd);
        printf("getaddrinfo failed: %s\n", gai_strerror(rc));
        exit(EXIT_FAILURE);
    }

    /* Connect to the server. */
    if (connect(conn_fd, res->ai_addr, res->ai_addrlen) < 0) 
    {
        perror_exit("connect()");
    }
    printf("Connected\n");

    /* Send messages to server through thread. */
    pthread_t send_thread;
    if (pthread_create(&send_thread, NULL, send_routine, NULL) != 0)
    {
        perror_exit("pthread_create()");
    }

    /* Receive message from server through thread. */
    pthread_t recv_thread;
    if (pthread_create(&recv_thread, NULL, recv_routine, NULL) != 0)
    {
        perror_exit("pthread_create()");
    }

    /* Wait for both threads. */
    pthread_join(send_thread, NULL);
    pthread_join(recv_thread, NULL);

    return 0;
}

/* Function Implementations */

/* Call perror on failed command, close 
   the conn_fd socket, and exit the 
   current process with EXIT_FAILURE. */

static inline void perror_exit(char *cmd)
{
    perror(cmd);
    close(conn_fd);
    exit(EXIT_FAILURE);
}

/* Send messages to server. */
 
void *send_routine()
{
    int bytes_read;
    char buf[BUF_SIZE + 1];

    while (1) 
    {
        /* Read bytes from user input. */
        if ((bytes_read = read(STDIN_FILENO, buf, BUF_SIZE)) < 0)
        {   
            perror_exit("read()");
        }

        /* Client exits with Ctrl-D or inputs "exit". */
        if (bytes_read == 0 || strncmp(buf, "/quit\n", 6) == 0)
        {
            printf("Exiting.\n"); 
            close(conn_fd);
            exit(CLIENT_CLOSED);
        }

        /* Send data to server. */
        buf[bytes_read] = '\0';
        send(conn_fd, buf, bytes_read + 1, 0);
    }
    return NULL;
}

/* Receive messages from server. */

void *recv_routine()
{
    int bytes_received;
    char buf[BUF_SIZE + 1];

    while (1) 
    {
        /* Receive bytes from server. */
        if ((bytes_received = recv(conn_fd, buf, BUF_SIZE, 0)) < 0)
        {
            perror_exit("recv()");
        }

        /* Server closed. */
        if (bytes_received == 0)
        {
            printf("Connection closed by remote host.\n");
            close(conn_fd);
            exit(SERVER_CLOSED);
        }

        /* Received data from server. */
        buf[bytes_received] = '\0';
        printf("%s", buf);
        fsync(STDOUT_FILENO);
    }
    return NULL;
}