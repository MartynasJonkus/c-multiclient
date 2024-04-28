#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define EMAIL_PORT 10101

pthread_t threads[MAX_CLIENTS];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int client_count = 0;
int client_sockets[MAX_CLIENTS];
char client_usernames[MAX_CLIENTS][20];
int email_server_socket; // Global email server socket
int server_number = 1;

void send_message(int socket, const char *message);
void removeInvisibleChars(char *str);
void forward_message_to_email(const char *client_name, const char *message);

void *handle_client(void *arg)
{
    int client_index = *((int *)arg);
    int client_socket = client_sockets[client_index];
    char buffer[BUFFER_SIZE];

    while (1)
    {
        // Send "ATSIUSKVARDA" message to the client
        send_message(client_socket, "ATSIUSKVARDA");

        // Get client's username
        fgets(buffer, sizeof(buffer), fdopen(client_socket, "r"));
        removeInvisibleChars(buffer);

        printf("Received username from client: %s\n", buffer);

        // Lock the mutex before checking for unique name
        pthread_mutex_lock(&mutex);

        int is_unique = 1;
        for (int i = 0; i < client_count; i++)
        {
            if (i != client_index && strcmp(client_usernames[i], buffer) == 0)
            {
                is_unique = 0;
                break;
            }
        }

        if (is_unique)
        {
            // Client submitted a unique name, update the username
            strcpy(client_usernames[client_index], buffer);

            // Send acknowledgment to the client
            send_message(client_socket, "VARDASOK");

            printf("Unique username accepted for client: %s\n", buffer);

            // Unlock the mutex after updating shared data
            pthread_mutex_unlock(&mutex);

            break; // Exit the loop as the name is unique
        }

        // Unlock the mutex if the name is not unique
        pthread_mutex_unlock(&mutex);

        // Send a message to the client to submit a new name
        send_message(client_socket, "VARDASDUPLIKATAS");
        printf("Duplicate username detected for client: %s\n", buffer);
    }

    while (1)
    {
        // Receive message from the client
        fgets(buffer, sizeof(buffer), fdopen(client_socket, "r"));
        if (strlen(buffer) > 0)
        {
            forward_message_to_email(client_usernames[client_index], buffer);
        }
        memset(buffer, 0, sizeof(buffer));

        // Receive messages from email server
        ssize_t bytes_received = recv(email_server_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0)
        {
            // Handle error or connection closed
            break;
        }
        buffer[bytes_received] = '\0';

        // Forward the message to the client
        send_message(client_socket, buffer);
        printf("Forwarded message from email server to client: %s\n", buffer);
        memset(buffer, 0, sizeof(buffer));
    }

    return NULL;
}

void forward_message_to_email(const char *client_name, const char *message)
{
    // Format the message with client's name
    char formatted_message[BUFFER_SIZE];
    snprintf(formatted_message, BUFFER_SIZE, "%s@S%d %s", client_name, server_number, message);

    // Send the formatted message to the email server
    removeInvisibleChars(formatted_message);
    send(email_server_socket, formatted_message, strlen(formatted_message), 0);
    printf("Sent message to email server: %s\n", formatted_message);
}

void send_message(int socket, const char *message)
{
    dprintf(socket, "%s\n", message);
}

void removeInvisibleChars(char *str)
{
    int i, j = 0;

    for (i = 0; str[i] != '\0'; i++)
    {
        // Check if the character is visible
        if (!isspace((unsigned char)str[i]) || str[i] == ' ')
        {
            // If visible, copy it to the new position
            str[j++] = str[i];
        }
    }

    // Add null terminator to the end of the modified string
    str[j] = '\0';
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <server_number> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    server_number = atoi(argv[1]);
    int port = atoi(argv[2]);

    int server_socket, new_socket;
    struct sockaddr_storage server_addr, new_addr;
    socklen_t addr_size;

    server_socket = socket(AF_INET6, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("Error creating server socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.ss_family = AF_INET6;
    ((struct sockaddr_in6 *)&server_addr)->sin6_port = htons(port);
    ((struct sockaddr_in6 *)&server_addr)->sin6_addr = in6addr_any;

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_CLIENTS) == 0)
    {
        printf("Server listening on port %d...\n", port);
    }
    else
    {
        perror("Error listening for clients");
        exit(EXIT_FAILURE);
    }

    // Create email server socket
    email_server_socket = socket(AF_INET6, SOCK_STREAM, 0);
    if (email_server_socket < 0)
    {
        perror("Email Server Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up the address for the email server
    struct sockaddr_in6 email_server_addr;
    char email_server_ip[] = "::1";     // Change this to the IP address of the email server if it's not local
    int email_server_port = EMAIL_PORT; // Change this to the port of the email server

    memset(&email_server_addr, 0, sizeof(email_server_addr));
    email_server_addr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, email_server_ip, &email_server_addr.sin6_addr);
    email_server_addr.sin6_port = htons(email_server_port);

    // Connect to the email server
    if (connect(email_server_socket, (struct sockaddr *)&email_server_addr, sizeof(email_server_addr)) < 0)
    {
        perror("Email Server Connection failed");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        addr_size = sizeof(new_addr);
        new_socket = accept(server_socket, (struct sockaddr *)&new_addr, &addr_size);

        if (new_socket < 0)
        {
            perror("Error accepting connection");
            exit(EXIT_FAILURE);
        }

        char client_ip[INET6_ADDRSTRLEN];

        if (new_addr.ss_family == AF_INET)
        {
            struct sockaddr_in *addr_in = (struct sockaddr_in *)&new_addr;
            inet_ntop(AF_INET, &(addr_in->sin_addr), client_ip, INET_ADDRSTRLEN);
        }
        else if (new_addr.ss_family == AF_INET6)
        {
            struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)&new_addr;
            inet_ntop(AF_INET6, &(addr_in6->sin6_addr), client_ip, INET6_ADDRSTRLEN);
        }
        else
        {
            fprintf(stderr, "Unsupported address family\n");
            close(new_socket);
            continue;
        }

        printf("New client connected from %s\n", client_ip);

        // Lock the mutex before updating shared data
        pthread_mutex_lock(&mutex);

        // Add new client socket to the list
        client_sockets[client_count] = new_socket;

        // Create a new variable for each thread
        int current_client_count = client_count;

        // Increment the client_count for the next connection
        client_count++;

        // Unlock the mutex after updating shared data
        pthread_mutex_unlock(&mutex);

        // Create a new thread to handle the client, passing the new variable
        pthread_create(&threads[current_client_count], NULL, handle_client, (void *)&current_client_count);
    }

    close(server_socket);
    close(email_server_socket); // Close email server socket when server exits

    return 0;
}
