/*
 * subscriber_tcp.c
 *
 * Suscriptor TCP simple: se conecta al broker y envía el comando
 * "SUBSCRIBE <topic>". Después de suscribirse, espera en un bucle leyendo
 * del socket TCP e imprime cualquier mensaje reenviado por el broker para
 * el topic suscrito.
 *
 * Uso de librerías:
 * - inet_pton() (<arpa/inet.h>) para preparar la dirección remota.
 * - socket/connect/read/send/close (API de sockets POSIX) para interactuar
 *   con el broker TCP. La API de sockets está en <sys/socket.h> y close/read
 *   vienen de <unistd.h>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    char topic[50];

    /* Crear socket TCP */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error creando socket");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    /* Convertir la cadena IP a la representación binaria */
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("Dirección inválida");
        return -1;
    }

    /* Conectar con el broker. Si tiene éxito, se pueden enviar comandos y
     * leer los mensajes reenviados desde el mismo socket. */
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Conexión fallida");
        return -1;
    }

    printf("Suscriptor conectado al broker TCP.\n");
    printf("Tema al que deseas suscribirte: ");
    scanf("%s", topic);

    char subscribe_msg[BUFFER_SIZE];
    sprintf(subscribe_msg, "SUBSCRIBE %s", topic);
    send(sock, subscribe_msg, strlen(subscribe_msg), 0);

    printf("Esperando mensajes del tema '%s'...\n", topic);
    while (1) {
        int valread = read(sock, buffer, BUFFER_SIZE - 1);
        if (valread > 0) {
            buffer[valread] = '\0';
            printf("%s\n", buffer);
        }
    }

    close(sock);
    return 0;
}