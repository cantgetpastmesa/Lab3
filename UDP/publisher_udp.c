/*
 * publisher_udp.c
 *
 * Publicador UDP simple que envía datagramas al broker. Como UDP es
 * sin conexión, el publicador no llama a connect(); en su lugar usa
 * sendto() especificando la dirección del broker.
 *
 * Uso:
 * - socket(AF_INET, SOCK_DGRAM, 0) para crear un socket UDP IPv4.
 * - inet_pton() para convertir la IP en forma binaria para sockaddr_in.
 * - sendto() para transmitir datagramas al broker.
 */

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
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];

    /* Crear socket UDP */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error creando socket UDP");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr);

    printf("Publisher UDP iniciado.\n");
    printf("Formato: tema|mensaje (ejemplo: partido1|Gol minuto 45)\n");

    while (1) {
        printf("> ");
        fgets(buffer, BUFFER_SIZE, stdin);
    buffer[strcspn(buffer, "\n")] = 0; /* eliminar salto de línea */

        if (strcmp(buffer, "exit") == 0)
            break;

    /* sendto() envía un datagrama UDP al broker */
        sendto(sockfd, buffer, strlen(buffer), 0,
               (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    }

    close(sockfd);
    return 0;
}