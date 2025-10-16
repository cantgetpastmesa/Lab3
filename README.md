Laboratorio 3 - ANÁLISIS DE CAPA DE TRANSPORTE Y SOCKETS - Infraestructura de Comunicaciones 
====================

Este repositorio contiene una práctica de comunicación tipo *publish/subscribe* implementada en C, utilizando los protocolos TCP y UDP. 
El propósito del laboratorio es comprender cómo se implementan mecanismos básicos de publicación y suscripción en la capa de aplicación, 
y observar las diferencias prácticas entre un protocolo orientado a conexión (TCP) y uno no orientado a conexión (UDP).

El repositorio incluye implementaciones de *broker*, *publisher* y *subscriber* para cada protocolo:

- **TCP/**
  - `broker_tcp.c`: actúa como intermediario central; acepta conexiones, gestiona suscripciones y reenvía mensajes a los clientes suscritos.
  - `publisher_tcp.c`: se conecta al *broker* y envía mensajes con el formato `topic|message`.
  - `subscriber_tcp.c`: se conecta al *broker*, envía `SUBSCRIBE <topic>` y muestra los mensajes reenviados.
- **UDP/**
  - `broker_udp.c`: recibe datagramas, registra las direcciones de los suscriptores y reenvía los mensajes.
  - `publisher_udp.c`: envía datagramas en formato `topic|message`.
  - `subscriber_udp.c`: enlaza un puerto efímero, envía `SUBSCRIBE <topic>` al *broker* y espera datagramas entrantes.

Funcionalidad por protocolo
===========================

**TCP**  
El *broker* TCP crea un socket de escucha con `socket(..., SOCK_STREAM, ...)`, seguido de `bind()`, `listen()` y `select()` 
para manejar múltiples conexiones simultáneas. Los clientes se conectan con `connect()`.  
Un suscriptor envía la cadena `SUBSCRIBE <topic>` al *broker*, quien registra su socket y el tema en una estructura `Subscriber`.

Cuando un publicador envía un mensaje en formato `topic|message`, el *broker* lo separa en ambos campos y lo reenvía 
mediante `send()` a todos los suscriptores cuyo tema coincida.

**Encabezados utilizados:**
- `<sys/socket.h>`: declara las funciones principales de la API de sockets (socket, bind, listen, accept, send, recv).
- `<arpa/inet.h>`: define estructuras y funciones de conversión como `struct sockaddr_in`, `htons()` e `inet_pton()`.
- `<sys/select.h>`: permite el uso de `select()` y los conjuntos de descriptores `fd_set`, utilizados para multiplexar sockets.

**UDP**  
En este caso, el *broker* UDP usa `socket(..., SOCK_DGRAM, ...)` y `bind()`. 
No existe una conexión persistente: el *broker* utiliza `recvfrom()` para recibir datagramas y conocer la dirección del remitente.  
Si el mensaje recibido comienza con `SUBSCRIBE <topic>`, almacena la dirección del remitente junto con el tema.  
Cuando un publicador envía `topic|message`, el *broker* utiliza `sendto()` para reenviar el datagrama a todos los suscriptores registrados.

**Encabezados (librerías) utilizados:**
- `<sys/socket.h>`: proporciona las funciones `sendto()` y `recvfrom()`, necesarias para enviar y recibir datagramas.
- `<arpa/inet.h>`: nuevamente usada para manipular direcciones IPv4 y conversiones de red.

Flujo de interacción
====================

**TCP:**
1. El *broker* escucha en un puerto definido.
2. Los suscriptores se conectan y envían su comando de suscripción (`SUBSCRIBE <topic>`).
3. Los publicadores se conectan y envían mensajes con el formato `topic|message`.
4. El *broker* reenvía los mensajes a todos los suscriptores registrados en el mismo tema.

**UDP:**
1. El *broker* escucha datagramas en el puerto configurado.
2. Los suscriptores envían `SUBSCRIBE <topic>` al *broker* y registran su dirección local.
3. Los publicadores envían datagramas `topic|message`.
4. El *broker* reenvía los datagramas a los suscriptores que coincidan con el tema.

Ejemplos y fragmentos clave
===========================

1) **Creación de socket:**
   - Servidor TCP: `server_fd = socket(AF_INET, SOCK_STREAM, 0);`
   - Socket UDP: `sockfd = socket(AF_INET, SOCK_DGRAM, 0);`

2) **Configuración de dirección:**
   ```c
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_port = htons(PORT);
   inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr);
   ```

3) **Recepción de datagrama UDP:**
   ```c
   recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
            (struct sockaddr *)&client_addr, &addr_len);
   ```

4) **Reenvío UDP a suscriptores:**
   ```c
   sendto(sockfd, msg, strlen(msg), 0,
          (struct sockaddr *)&subs[i].addr, sizeof(subs[i].addr));
   ```


Cómo correrlo (más para que nosotros recordemos, los asistentes obvio saben cómo)
================

Compilar y ejecutar cada módulo en terminales separadas:

```bash
# Broker TCP
gcc TCP/broker_tcp.c -o broker_tcp
./broker_tcp

# Broker UDP
gcc UDP/broker_udp.c -o broker_udp
./broker_udp
```

Luego, iniciar los clientes:

```bash
# Suscriptor TCP
gcc TCP/subscriber_tcp.c -o subscriber_tcp
./subscriber_tcp

# Publicador TCP
gcc TCP/publisher_tcp.c -o publisher_tcp
./publisher_tcp

# Suscriptor UDP
gcc UDP/subscriber_udp.c -o subscriber_udp
./subscriber_udp

# Publicador UDP
gcc UDP/publisher_udp.c -o publisher_udp
./publisher_udp
```

Conclusión
==========

El código demuestra de forma práctica las diferencias entre comunicación confiable (TCP) y no confiable (UDP) dentro de un modelo de publicación/suscripción.
Cada implementación hace uso directo de las llamadas al sistema POSIX para manejo de sockets, reforzando los conceptos de programación en red a bajo nivel.
