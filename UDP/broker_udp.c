/*
 * broker_udp.c
 *
 * Broker basado en UDP. A diferencia de TCP, UDP es sin conexión: publishers
 * y subscribers envían datagramas al puerto UDP del broker. Los subscribers
 * se registran enviando "SUBSCRIBE <topic>" desde su dirección UDP. Cuando
 * un publisher envía "topic|message", el broker reenvía el mensaje usando
 * sendto() a las direcciones de los subscribers registrados.
 *
 * Encabezados no estándar clave:
 * - <arpa/inet.h>: define sockaddr_in y helpers como htons/inet_pton.
 * - <sys/socket.h>: prototipos de socket(), bind(), recvfrom(), sendto().
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_SUBS 50

/* Entrada de suscriptor para UDP
 * - addr: sockaddr_in que contiene la IP y el puerto del endpoint UDP del suscriptor.
 *         En UDP debemos recordar la dirección del cliente para poder
 *         enviarle datagramas de vuelta con sendto().
 * - topic: cadena con el topic al que el cliente se suscribió.
 */
typedef struct {
    struct sockaddr_in addr;
    char topic[50];
} Subscriber;

Subscriber subscribers[MAX_SUBS];
int sub_count = 0;

/* add_subscriber
 * - Añade la dirección y el topic del cliente a la lista de suscriptores.
 * - Comprueba entradas existentes para evitar suscripciones duplicadas
 *   desde el mismo par (IP, puerto)/topic.
 */
void add_subscriber(struct sockaddr_in addr, const char *topic) {
    for (int i = 0; i < sub_count; i++) {
        if (strcmp(subscribers[i].topic, topic) == 0 &&
            subscribers[i].addr.sin_addr.s_addr == addr.sin_addr.s_addr &&
            subscribers[i].addr.sin_port == addr.sin_port)
            return; // ya suscrito
    }
    subscribers[sub_count].addr = addr;
    strcpy(subscribers[sub_count].topic, topic);
    sub_count++;
    printf("Nuevo suscriptor al tema: %s\n", topic);
}

/* send_to_subscribers
 * - Para cada suscriptor cuyo topic coincide con 'topic', envía el mensaje
 *   usando sendto(). sendto() recibe una sockaddr de destino explícita y
 *   es la llamada adecuada para datagramas UDP (declarada en <sys/socket.h>). */
void send_to_subscribers(int sockfd, const char *topic, const char *msg) {
    for (int i = 0; i < sub_count; i++) {
        if (strcmp(subscribers[i].topic, topic) == 0) {
            sendto(sockfd, msg, strlen(msg), 0,
                   (struct sockaddr *)&subscribers[i].addr, sizeof(subscribers[i].addr));
        }
    }
}

int main() {
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in broker_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    /* Crear socket UDP */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error creando socket UDP");
        exit(EXIT_FAILURE);
    }

    broker_addr.sin_family = AF_INET;
    broker_addr.sin_addr.s_addr = INADDR_ANY;
    broker_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&broker_addr, sizeof(broker_addr)) < 0) {
        perror("Error en bind");
        exit(EXIT_FAILURE);
    }

    printf("Broker UDP escuchando en puerto %d...\n", PORT);

    while (1) {
        int len = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                           (struct sockaddr *)&client_addr, &addr_len);
        if (len < 0) continue;

        buffer[len] = '\0';

        if (strncmp(buffer, "SUBSCRIBE", 9) == 0) {
            char topic[50];
            sscanf(buffer, "SUBSCRIBE %s", topic);
            add_subscriber(client_addr, topic);
        } else {
            char topic[50], msg[BUFFER_SIZE];
            char *sep = strchr(buffer, '|');
            if (sep) {
                *sep = '\0';
                strcpy(topic, buffer);
                strcpy(msg, sep + 1);
                printf("Mensaje recibido del tema '%s': %s\n", topic, msg);
                send_to_subscribers(sockfd, topic, msg);
            }
        }
    }

    close(sockfd);
    return 0;
}