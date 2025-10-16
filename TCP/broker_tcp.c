/*
 * broker_tcp.c
 *
 * Este archivo implementa un broker TCP simple que permite que clientes se
 * conecten como publishers o subscribers. Los publishers envían mensajes en
 * el formato "topic|message". Los subscribers envían el comando
 * "SUBSCRIBE <topic>" después de conectarse y el broker reenvía los mensajes
 * coincidentes.
 *
 * Encabezados no estándar usados y su papel (detalles abajo):
 * - <arpa/inet.h>  : funciones para conversión de direcciones IP
 *                    (inet_pton) y la estructura sockaddr_in usada para
 *                    especificar direcciones IPv4.
 * - <sys/socket.h> : API de sockets (socket, bind, listen, accept, send, recv).
 * - <sys/select.h> : select() y macros fd_set para multiplexar E/S entre sockets.
 * - <unistd.h>     : close(), read(), write() y llamadas POSIX varias.
 * - <errno.h>      : constantes errno usadas en el manejo de errores de select().
 *
 * Contrato (entradas/salidas):
 * - Entradas: conexiones TCP de publishers y subscribers en PORT.
 * - Salidas: reenvía mensajes recibidos de publishers a los subscribers
 *           que se hayan suscrito al topic correspondiente.
 * - Modos de error: fallos en sockets causan salida del programa; desconexiones
 *                  de clientes se manejan cerrando el descriptor y eliminándolo
 *                  de los arreglos internos.
 */

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

/*
 * Estructura Subscriber
 * - socket: descriptor de fichero del cliente TCP conectado (suscriptor).
 * - topic:  cadena terminada en nulo que identifica el topic al que el cliente
 *          se suscribió.
 *
 * Nota: Esta implementación asume una suscripción por conexión.
 */
typedef struct {
    int socket;
    char topic[50];
} Subscriber;

/* Estado global */
Subscriber subscribers[MAX_CLIENTS]; // arreglo que contiene los registros de suscriptores
int sub_count = 0;                   // número de suscriptores activos

/*
 * add_subscriber
 * - Registra un socket como suscriptor de un topic.
 * - Copia la cadena del topic y guarda el FD del socket. No se realizan
 *   comprobaciones de unicidad más allá de la capacidad.
 */
void add_subscriber(int sock, const char *topic) {
    if (sub_count < MAX_CLIENTS) {
        subscribers[sub_count].socket = sock;
        strcpy(subscribers[sub_count].topic, topic);
        sub_count++;
        printf("Nuevo suscriptor del tema: %s\n", topic);
    }
}

/*
 * send_to_subscribers
 * - Itera sobre los suscriptores registrados y envía 'message' a aquellos
 *   cuyo topic coincide con el topic proporcionado. Usa la llamada POSIX
 *   'send' (de <sys/socket.h>). Para sockets TCP, send() escribe los datos
 *   en la corriente TCP conectada al cliente remoto.
 */
void send_to_subscribers(const char *topic, const char *message) {
    for (int i = 0; i < sub_count; i++) {
        if (strcmp(subscribers[i].topic, topic) == 0) {
            send(subscribers[i].socket, message, strlen(message), 0);
        }
    }
}

/*
 * main
 * - Crea un socket TCP en modo escucha, acepta conexiones entrantes y
 *   las multiplexa usando select(). Cuando un cliente envía datos, el
 *   servidor lee e interpreta comandos:
 *     - "SUBSCRIBE <topic>" registra al cliente como suscriptor
 *     - "<topic>|<message>" se considera una publicación y el broker
 *         reenvía <message> a todos los suscriptores de <topic>.
 *
 * Interacciones clave con encabezados no estándar:
 * - socket(AF_INET, SOCK_STREAM, 0): crea un socket TCP IPv4 (sys/socket.h).
 * - bind(...): enlaza el socket a INADDR_ANY y PORT usando sockaddr_in
 *              (arpa/inet.h define la estructura y helpers como htons()).
 * - listen(), accept(): ponen el socket en modo escucha y aceptan conexiones
 *                        TCP (sys/socket.h).
 * - select(): provisto por <sys/select.h>, permite esperar en múltiples
 *             sockets para lectura. Aquí se usa para saber cuándo hay datos
 *             disponibles en cualquier socket conectado o cuando llega una
 *             nueva conexión en el socket escuchante.
 * - read()/close(): helpers POSIX de <unistd.h> para recibir bytes y cerrar
 *                   sockets cuando los clientes se desconectan.
 */
int main() {
    int server_fd, new_socket, max_sd, sd, activity;
    struct sockaddr_in address; /* address for bind/accept functions */
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];
    fd_set readfds;                        /* for select() */
    int client_sockets[MAX_CLIENTS] = {0}; /* tracked connected client FDs */

    /* Crear socket TCP: AF_INET (IPv4), SOCK_STREAM (TCP) */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Error creando socket");
        exit(EXIT_FAILURE);
    }

    /* Configurar dirección: INADDR_ANY escucha en todas las interfaces */
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT); /* convert port to network byte order */

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

        /* Register client sockets into the read set for select() */
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];
            if (sd > 0)
                FD_SET(sd, &readfds);
            if (sd > max_sd)
                max_sd = sd;
        }

        /* select bloqueará hasta que ocurra actividad en algún FD en readfds */
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            perror("Error en select");
        }

    /* Si el socket escuchante es legible, hay una nueva conexión */
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

        /* Check each client socket for incoming data */
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];
            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                int valread = read(sd, buffer, BUFFER_SIZE - 1);
                if (valread == 0) {
                    /* Cliente desconectado */
                    close(sd);
                    client_sockets[i] = 0;
                } else {
                    buffer[valread] = '\0';
                    if (strncmp(buffer, "SUBSCRIBE", 9) == 0) {
                        char topic[50];
                        sscanf(buffer, "SUBSCRIBE %s", topic);
                        add_subscriber(sd, topic);
                    } else {
                        /* Se espera el formato "topic|message" para publicaciones */
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