// broker_tcp.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define PORT 8080
#define MAX_CLIENTS 20
#define BUFFER_SIZE 1024

typedef struct {
    int socket;
    char topic[50];
} Subscriber;

Subscriber subscribers[MAX_CLIENTS];
int sub_count = 0;

void add_subscriber(int sock, const char *topic) {
    if (sub_count < MAX_CLIENTS) {
        subscribers[sub_count].socket = sock;
        strcpy(subscribers[sub_count].topic, topic);
        sub_count++;
        printf("Nuevo suscriptor del tema: %s\n", topic);
    }
}

void send_to_subscribers(const char *topic, const char *message) {
    for (int i = 0; i < sub_count; i++) {
        if (strcmp(subscribers[i].topic, topic) == 0) {
            send(subscribers[i].socket, message, strlen(message), 0);
        }
    }
}

int main() {
    int server_fd, new_socket, max_sd, sd, activity;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];
    fd_set readfds;
    int client_sockets[MAX_CLIENTS] = {0};

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Error creando socket");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Error en bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("Error en listen");
        exit(EXIT_FAILURE);
    }

    printf("Broker TCP escuchando en el puerto %d...\n", PORT);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];
            if (sd > 0)
                FD_SET(sd, &readfds);
            if (sd > max_sd)
                max_sd = sd;
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            perror("Error en select");
        }

        if (FD_ISSET(server_fd, &readfds)) {
            new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
            printf("Nueva conexión establecida.\n");

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];
            if (FD_ISSET(sd, &readfds)) {
                int valread = read(sd, buffer, BUFFER_SIZE - 1);
                if (valread == 0) {
                    close(sd);
                    client_sockets[i] = 0;
                } else {
                    buffer[valread] = '\0';
                    if (strncmp(buffer, "SUBSCRIBE", 9) == 0) {
                        char topic[50];
                        sscanf(buffer, "SUBSCRIBE %s", topic);
                        add_subscriber(sd, topic);
                    } else {
                        char topic[50], msg[BUFFER_SIZE];
                        char *sep = strchr(buffer, '|');
                        if (sep) {
                            *sep = '\0';
                            strcpy(topic, buffer);
                            strcpy(msg, sep + 1);
                            printf("Mensaje recibido del tema '%s': %s\n", topic, msg);
                            send_to_subscribers(topic, msg);
                        }
                    }
                }
            }
        }
    }
}