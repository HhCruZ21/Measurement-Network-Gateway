#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "../include/headers.h"
#include "../include/protocol.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5000

static ssize_t recvAll(int fd, void *buf, size_t len)
{
    size_t total = 0;
    uint8_t *p = buf;

    while (total < len)
    {
        ssize_t r = recv(fd, p + total, len - total, 0);
        if (r <= 0)
            return -1;
        total += r;
    }
    return total;
}

int main(void)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(SERVER_PORT)};

    inet_pton(AF_INET, SERVER_IP, &addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("connect");
        return 1;
    }

    printf("[CLIENT] Connected to %s:%d\n", SERVER_IP, SERVER_PORT);

    while (1)
    {
        protocol_header_t hdr;
        size_t i;

        /* ---- 1. Receive protocol header ---- */
        if (recvAll(sock, &hdr, sizeof(hdr)) < 0)
        {
            printf("[CLIENT] Server disconnected\n");
            break;
        }

        /* ---- 2. Validate protocol ---- */
        if(hdr.mgc != PROTOCOL_MAGIC || hdr.version != PROTOCOL_VERSION){
            printf("[CLIENT] Protocol mismatch\n");
            break;
        }

        /* ---- 3. Handle heartbeat ---- */
        if(hdr.type == MSG_HEARTBEAT){
            printf("[CLIENT] Heartbeat received\n");
            continue;
        }

        /* ---- 4. Handle samples ---- */
        if(hdr.type != MSG_SAMPLES){
            printf("[CLIENT] Unknown message type\n");
            break;
        }

        if (hdr.payload_len == 0 || hdr.payload_len % sizeof(sample_t) != 0)
        {
            printf("[CLIENT] Invalid payload length: %u\n", hdr.payload_len);
            break;
        }

        size_t sampleCount = hdr.payload_len / sizeof(sample_t);

        sample_t *samples = malloc(hdr.payload_len);
        if (!samples)
        {
            perror("malloc");
            break;
        }

        /* ---- 5. Receive payload ---- */
        if (recvAll(sock, samples, hdr.payload_len) < 0)
        {
            printf("[CLIENT] Payload receive failed\n");
            free(samples);
            break;
        }

        /* ---- 6. Process samples ---- */
        for (i = 0; i < sampleCount; i++)
        {
            printf("ID=%u, TS=%llu us, VAL+%d\n",
                   samples[i].sensorId,
                   (unsigned long long)samples[i].timestampUs,
                   samples[i].value);
        }
        free(samples);
    }
    close(sock);
    return 0;
}