/*
*   chat-server.c
*/

/* Header Files */ 
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

/* Client Struct */
struct Client
{
    int conn_fd;                
    char *name;
    char *ip;                 
    uint16_t port;
    struct Client* next;
    struct Client* prev;
};

/* Constants and Global Vars */ 
#define BACKLOG  100
#define BUF_SIZE 4096
static struct Client *head = NULL;
static pthread_mutex_t list_mutex;

/* Function Prototypes */ 
static inline void perror_exit(char *cmd);
int prefix_time_to_msg(char *msg, char *dest, size_t dest_size);
void *client_routine(void *client_arg);
void *create_and_append_client(int conn_fd, char *ip, uint16_t port); 
void delete_and_free_client(struct Client *client); 
void send_all_clients(char *message, size_t msg_length);
void close_handler(int sig);
 
/* Main */ 

int main(int argc, char *argv[])
{
    char *listen_port, *remote_ip;
    int listen_fd, conn_fd, rc;
    uint16_t remote_port;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in remote_sa;
    struct sigaction sa;

    /* Initialize mutex for linked-list. */
    pthread_mutex_init(&list_mutex, NULL);
 
    /* Set up sigaction struct. */
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = close_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
 
    /* Associate action with signal SIGINT (ctrl-C). */
    if (sigaction(SIGINT, &sa, NULL) < 0) 
    {
        perror_exit("sigaction()");
    }

    /* Cmd-line arg 1 specifies server port. */
    listen_port = argv[1];

    /* Create a socket. */
    if ((listen_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror_exit("socket()");
    }
 
    /* Bind to a port. */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rc = getaddrinfo(NULL, listen_port, &hints, &res)) != 0) 
    {
        close(listen_fd);
        printf("getaddrinfo failed: %s\n", gai_strerror(rc));
        exit(EXIT_FAILURE);
    }
    if ((bind(listen_fd, res->ai_addr, res->ai_addrlen)) < 0)
    {
        close(listen_fd);
        perror_exit("bind()");
    }

    /* Start listening. */
    if (listen(listen_fd, BACKLOG) < 0)
    {
        close(listen_fd);
        perror_exit("listen()");
    }
 
    /* Accept new connections and handle them until server closes. */
    while (1) 
    {
        /* Accept a new connection (will block until one appears). */
        addrlen = sizeof(remote_sa);
        if ((conn_fd = accept(listen_fd, (struct sockaddr *) &remote_sa, &addrlen)) < 0)
        {
            perror("accept()");
            close(conn_fd);
            continue;
        }

        /* Announce our communication partner. */
        remote_ip = inet_ntoa(remote_sa.sin_addr);
        remote_port = ntohs(remote_sa.sin_port);
        printf("new connection from %s:%d\n", remote_ip, remote_port);

        /* Create new Client and corresponding thread. */
        pthread_t thread_id;
        struct Client* new_client; 
        
        pthread_mutex_lock(&list_mutex);
        if ((new_client = create_and_append_client(conn_fd, remote_ip, remote_port)) == NULL)
        {
            printf("Failed to create new client.\n");
            close(conn_fd); 
        }
        else
        {
            if (pthread_create(&thread_id, NULL, &client_routine, (void *)new_client) != 0)
            {
                perror("pthread_create");
                delete_and_free_client(new_client);
            }
        }
        pthread_mutex_unlock(&list_mutex);
    }

    /* Close socket and destroy mutex. */
    close(listen_fd);
    pthread_mutex_destroy(&list_mutex);
    return 0;
}

/* Function Implementations */

/* Call perror on failed command and exit
   the current process with EXIT_FAILURE. */

static inline void perror_exit(char *cmd)
{
    perror(cmd);
    exit(EXIT_FAILURE);
}

/* Prefix the current time to a message
   and return the new prefixed-message length. */

int prefix_time_to_msg(char *msg, char *dest, size_t dest_size)
{
    time_t now;
    struct tm *time_data;
    char time_str[40];

    now = time(NULL);
    time_data = localtime(&now);
    strftime(time_str, sizeof(time_str), "%H:%M:%S", time_data);
    return snprintf(dest, dest_size, "%s: %s", time_str, msg);
}

/* Client Thread Routine. */ 

void *client_routine(void *client_arg)
{
    struct Client *client = (struct Client *)client_arg;
    char buf[BUF_SIZE], old_name[BUF_SIZE], server_msg[BUF_SIZE], client_msg[BUF_SIZE];
    int bytes_received;
    int msg_length = 0;
    char *token;

    while ((bytes_received = recv(client->conn_fd, buf, BUF_SIZE, 0)) > 0) 
    {
        /* User requests for a nickname change. */
        if (strncmp(buf, "/nick ", 6) == 0)
        {
            /* Get username. */
            token = strtok(buf, " ");
            token = strtok(NULL, "\n");

            /* Set up name change announcement. */
            if (client->name == NULL)
            {
                strncpy(old_name, token, strlen(token));
                client->name = old_name;
                msg_length = snprintf(server_msg, BUF_SIZE, "User unknown (%s:%d) is now known as %s.\n", client->ip, client->port, client->name);
            }
            else
            {
                msg_length = snprintf(server_msg, BUF_SIZE, "User %s (%s:%d) is now known as %s.\n", client->name, client->ip, client->port, token);
                strncpy(old_name, token, strlen(token)); 
            }

            /* Server Announcement. */
            printf("%s", server_msg);
        }
        else 
        {
            /* Regular message. */
            fsync(STDOUT_FILENO);

            if (client->name != NULL)
            {
                msg_length = snprintf(server_msg, BUF_SIZE, "%s: %s", client->name, buf);
            }
            else
            {
                msg_length = snprintf(server_msg, BUF_SIZE, "User unknown (%s:%d): %s", client->ip, client->port, buf);
            }
        }

        /* Send message to all clients, with time prefixed. */
        msg_length = prefix_time_to_msg(server_msg, client_msg, BUF_SIZE);
        pthread_mutex_lock(&list_mutex);
        send_all_clients(client_msg, msg_length);
        pthread_mutex_unlock(&list_mutex);
    }

    /* Recv failed. */
    if (bytes_received < 0)
    {
        perror("recv");
        return NULL;
    }

    /* Client lost connection (Ctrl-D) or input "exit". */ 
    if (bytes_received == 0)
    {
        if (client->name != NULL)
        {
            printf("Lost connection from %s\n", client->name);
            msg_length = snprintf(server_msg, BUF_SIZE, "User %s (%s:%d) has disconnected.\n", client->name, client->ip, client->port);
        }
        else
        {
            printf("Lost connection from user unknown (%s:%d)\n", client->ip, client->port);
            msg_length = snprintf(server_msg, BUF_SIZE, "User unknown (%s:%d) has disconnected.\n", client->ip, client->port);
        }
        msg_length = prefix_time_to_msg(server_msg, client_msg, BUF_SIZE);
        pthread_mutex_lock(&list_mutex);
        delete_and_free_client(client);
        send_all_clients(client_msg, msg_length);
        pthread_mutex_unlock(&list_mutex);
    }

    return NULL;
}

/* Create a new Client and append it
   to the end of the list of clients. 
   Return the new Client. */ 

void *create_and_append_client(int conn_fd, char *ip, uint16_t port) 
{
    struct Client *new_client; 
    struct Client *curr_client;

    /* Allocate memory for Client struct and initialize its members. */
    if ((new_client = (struct Client *) malloc(sizeof(struct Client))) == NULL)
    {
        perror("malloc()");
        return NULL;
    }
    new_client->conn_fd = conn_fd;
    new_client->ip      = ip;
    new_client->port    = port;
    new_client->next    = NULL;
    new_client->name    = NULL;

    /* If the Client list is empty, set as the first Client. */
    if (head == NULL) 
    {
        new_client->prev = NULL;
        head = new_client;
        return new_client;
    }

    /* If the Client list is not empty, append Client to the end. */
    curr_client = head;
    while (curr_client->next != NULL) 
    {
        curr_client = curr_client->next;
    }
    curr_client->next = new_client;
    new_client->prev  = curr_client;

    return new_client;
}
 
/* Delete a client, removing it from the 
   linked-list and freeing its allocation. */

void delete_and_free_client(struct Client *client) 
{
    /* Close connection. */
    close(client->conn_fd);

    /* Error checking. */ 
    if (head == NULL)
    {
        return;
    }

    /* Check if first client in list. */ 
    if (head == client)
    {
        head = client->next;
    }
    else
    {
        client->prev->next = client->next;
    }

    /* Check if not last client in list. */
    if (client->next != NULL)
    {
        client->next->prev = client->prev;
    }

    free(client);
}

/* Handler for a SIGINT signal the server. Closes all
   all connections to the server and frees all clients. */

void close_handler(int sig) 
{
    struct Client *curr_client = head;

    while (curr_client != NULL) 
    {
        close(curr_client->conn_fd);
        curr_client = curr_client->next;
        free(curr_client);
    }
    printf("\n");
    exit(EXIT_SUCCESS);
}

/* Send a message to every client. */ 

void send_all_clients(char *message, size_t msg_length)
{
    struct Client *curr_client = head; 
   
    while (curr_client != NULL) 
    {
        if (send(curr_client->conn_fd, message, msg_length, 0) < 0)
        {
            perror("send()");
        }
        curr_client = curr_client->next;
    }
}