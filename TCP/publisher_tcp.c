/*
 * publisher_tcp.c
 *
 * Publicador TCP: se conecta al socket del broker y envía líneas
 * introducidas por el usuario. El formato esperado es: "tópico|mensaje".
 *
 * Encabezados de red usados:
 * - <arpa/inet.h>: proporciona inet_pton() para convertir la IP textual
 *   "127.0.0.1" a la forma binaria necesaria para sockaddr_in.
 * - <sys/socket.h>: declara tipos de sockets y prototipos
 *   de send()/connect(). <unistd.h> provee close().
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

    /* Crear un socket TCP (IPv4) */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error creando socket");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    /* Convertir la IP textual a la forma binaria para sockaddr_in */
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("Dirección inválida");
        return -1;
    }

    /* Conectar con el broker: esto realiza el handshake TCP.
     * connect() es provisto por la API de sockets (<sys/socket.h>). */
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Conexión fallida");
        return -1;
    }

    printf("Publisher conectado al broker TCP.\n");
    printf("Formato: tema|mensaje (ejemplo: partido1|Gol minuto 32)\n");

    while (1) {
        printf("> ");
        fgets(buffer, BUFFER_SIZE, stdin);
    buffer[strcspn(buffer, "\n")] = 0; // eliminar salto de línea
        if (strcmp(buffer, "exit") == 0)
            break;
    /* send() escribe los datos en la corriente TCP hacia el broker */
        send(sock, buffer, strlen(buffer), 0);
    }

    close(sock);
    return 0;
}