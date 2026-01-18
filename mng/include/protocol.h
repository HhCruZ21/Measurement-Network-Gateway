#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define PROTOCOL_MAGIC 0x4D4E4748  // "MNGH"
#define PROTOCOL_VERSION 1

typedef enum {
    MSG_SAMPLES   = 1,
    MSG_HEARTBEAT = 2
} message_type_t;

/* Fixed-size header: 16 bytes */
typedef struct __attribute__((packed)) {
    uint32_t mgc;        // PROTOCOL_MAGIC
    uint16_t version;      // protocol version
    uint16_t type;         // message_type_t
    uint32_t payload_len;  // bytes after header
    uint32_t reserved;     // future use (CRC, flags, etc.)
} protocol_header_t;

#endif
