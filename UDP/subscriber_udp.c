// subscriber_udp.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in serv_addr, my_addr;
    char buffer[BUFFER_SIZE];
    char topic[50];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error creando socket UDP");
        return -1;
    }

    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(0); // puerto aleatorio

    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
        perror("Error en bind");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr);

    printf("Suscriptor UDP iniciado.\n");
    printf("Tema a suscribirse: ");
    scanf("%s", topic);

    char subscribe_msg[BUFFER_SIZE];
    sprintf(subscribe_msg, "SUBSCRIBE %s", topic);
    sendto(sockfd, subscribe_msg, strlen(subscribe_msg), 0,
           (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    printf("Esperando mensajes del tema '%s'...\n", topic);
    while (1) {
        int len = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, NULL, NULL);
        if (len > 0) {
            buffer[len] = '\0';
            printf("%s\n", buffer);
        }
    }

    close(sockfd);
    return 0;
}