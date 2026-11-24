/*
 * protocol.h - message format shared by the Zynq client and the Jetson server.
 *
 * Every message on the TCP connection is:
 *
 *     [ header, PROTO_HEADER_SIZE bytes ][ payload, payload_size bytes ]
 *
 * Header layout on the wire (all fields big-endian / network byte order):
 *
 *   offset  size  field
 *     0      4    magic           PROTO_MAGIC, marks the start of a message
 *     4      2    version         PROTO_VERSION
 *     6      2    type            one of msg_type_t
 *     8      4    payload_size    number of payload bytes after the header
 *    12      4    width           image width in pixels   (0 for text)
 *    16      4    height          image height in pixels  (0 for text)
 *    20      4    channels        1 = gray, 3 = RGB       (0 for text)
 *    24      4    crc32           CRC32 of the payload
 *    28      4    server_time_us  processing time on the server, in replies
 *                                 (microseconds, 0 otherwise)
 *
 * The header is never sent as a raw C struct: proto_pack() writes each field
 * byte by byte, so padding and byte order are the same on every machine.
 */
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#define PROTO_MAGIC        0x5A4A5331u                 /* "ZJS1" */
#define PROTO_VERSION      1
#define PROTO_HEADER_SIZE  32
#define PROTO_MAX_PAYLOAD  (64u * 1024u * 1024u)       /* 64 MB */
#define PROTO_DEFAULT_PORT 5000

typedef enum {
    MSG_TEXT         = 1,   /* client -> server: text file contents        */
    MSG_IMAGE        = 2,   /* client -> server: raw pixels (+ w, h, ch)    */
    MSG_TEXT_RESULT  = 3,   /* server -> client: uppercase text             */
    MSG_IMAGE_RESULT = 4,   /* server -> client: processed raw pixels       */
    MSG_ERROR        = 5    /* either way: payload is an error message      */
} msg_type_t;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t type;
    uint32_t payload_size;
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    uint32_t crc32;
    uint32_t server_time_us;
} msg_header_t;

/* ---- header encoding (no network involved) ---- */

/*
 * Check a received header: magic, version, known type, payload size,
 * and for image messages that width * height * channels == payload_size.
 * Returns ERR_OK, ERR_PROTO_MAGIC, ERR_PROTO_VERSION, ERR_PROTO_TYPE
 * or ERR_PROTO_SIZE.
 */
int proto_validate(const msg_header_t *hdr);

/* Standard CRC32 (same as zlib / Ethernet). CRC32("123456789") = 0xCBF43926. */
uint32_t crc32_compute(const void *data, size_t len);

/* Name of a message type, for log messages ("TEXT", "IMAGE", ...). */
const char *proto_type_name(uint16_t type);

/* ---- sending and receiving whole messages ---- */

/*
 * Send one message: header, then payload.
 * The caller fills info->type, width, height, channels and server_time_us;
 * magic, version, payload_size and crc32 are filled in here.
 * Returns ERR_OK, ERR_ARGS, ERR_PROTO_SIZE or ERR_SEND.
 */
int proto_send(int fd, const msg_header_t *info, const void *payload, uint32_t size);

/*
 * Receive one message: reads and validates the header, then the payload,
 * then checks the CRC32.
 * On success *payload is a malloc'ed buffer (caller frees), with one extra
 * '\0' after the data so text can be printed directly.
 * Returns ERR_OK, ERR_CLOSED, ERR_RECV, ERR_NOMEM, ERR_CRC
 * or one of the proto_validate errors.
 */
int proto_recv(int fd, msg_header_t *hdr, uint8_t **payload);

#endif /* PROTOCOL_H */