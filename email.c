#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define PORT 10101
#define FILENAME "received_messages.txt"

void *handle_client(void *arg);
void send_messages_to_client(const char *recipient_email, int client_socket);
void store_message(const char *message, int client_socket);

int main()
{
    int server_socket, client_socket;
    struct sockaddr_in6 server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Create server socket
    server_socket = socket(AF_INET6, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_addr = in6addr_any;
    server_addr.sin6_port = htons(PORT);

    // Bind server socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Email server listening on port %d...\n", PORT);

    // Accept connections from clients and create a new thread for each client
    while (1)
    {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0)
        {
            perror("Accept failed");
            continue;
        }

        printf("Client connected.\n");

        // Create a new thread to handle the client
        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, (void *)&client_socket) != 0)
        {
            perror("Thread creation failed");
            close(client_socket);
            continue;
        }
    }

    close(server_socket);
    return 0;
}

void *handle_client(void *arg)
{
    int client_socket = *((int *)arg);
    char buffer[BUFFER_SIZE];

    // Receive messages from client
    while (1)
    {
        ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0)
        {
            perror("Receive failed");
            break;
        }
        buffer[bytes_received] = '\0';

        // Process the received message (in this example, simply print it)
        printf("Received message from intermediary server: %s\n", buffer);

        // Check if message is a post request
        if (strstr(buffer, "#post") != NULL)
        {
            // Parse recipient email from the message
            char *recipient_email = strtok(buffer, " ");
            send_messages_to_client(recipient_email, client_socket);
        }
        else
        {
            store_message(buffer, client_socket);
        }
    }

    close(client_socket);
    return NULL;
}

void send_messages_to_client(const char *recipient_email, int client_socket)
{
    FILE *file = fopen(FILENAME, "r");
    if (file == NULL)
    {
        perror("Failed to open file");
        return;
    }

    char buffer[BUFFER_SIZE];
    char concatenated_messages[BUFFER_SIZE * 10] = ""; // Assuming maximum of 10 messages concatenated
    int total_bytes_sent = 0;

    while (fgets(buffer, BUFFER_SIZE, file) != NULL)
    {
        if (strstr(buffer, recipient_email) != NULL)
        {
            // Concatenate the message to the buffer
            strcat(concatenated_messages, buffer);
        }
    }

    // Send the concatenated messages to the client
    total_bytes_sent = send(client_socket, concatenated_messages, strlen(concatenated_messages), 0);

    if (total_bytes_sent < 0)
    {
        perror("Send failed");
    }
    else
    {
        printf("Sent %d bytes to client: %s\n", total_bytes_sent, concatenated_messages);
    }

    fclose(file);
}

void store_message(const char *message, int client_socket)
{
    FILE *file = fopen(FILENAME, "a");
    if (file == NULL)
    {
        perror("Failed to open file");
        return;
    }

    fprintf(file, "%s\n", message);
    printf("Stored message: %s\n", message);

    char *confirm_msg = "Message sent.\n";
    send(client_socket, confirm_msg, strlen(confirm_msg), 0);

    fclose(file);
}
