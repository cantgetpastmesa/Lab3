/*
 * subscriber_udp.c
 *
 * Suscriptor UDP que enlaza un socket UDP local (puerto efímero) y envía un
 * datagrama "SUBSCRIBE <topic>" al broker. El broker registra la dirección
 * del emisor (IP y puerto efímero) y luego envía datagramas de vuelta a
 * esa dirección cuando se publica un mensaje para el topic.
 *
 * Interacciones notables con librerías:
 * - bind(): utilizada para asignar una dirección/puerto local al socket UDP.
 *   Aquí usamos INADDR_ANY y puerto 0 para que el SO elija un puerto efímero.
 * - sendto(): envía el comando SUBSCRIBE a la dirección del broker.
 * - recvfrom(): bloquea esperando datagramas entrantes (mensajes del broker)
 *   y devuelve los bytes recibidos y la dirección del emisor si se solicita.
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
    struct sockaddr_in serv_addr, my_addr;
    char buffer[BUFFER_SIZE];
    char topic[50];

    /* Crear socket UDP */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error creando socket UDP");
        return -1;
    }

    /* Enlazar a cualquier interfaz local y permitir que el SO elija un puerto libre (puerto 0) */
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(0); // ephemeral port

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
    /* Enviar el comando SUBSCRIBE al broker para que sepa dónde entregar los mensajes */
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