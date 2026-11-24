#include "protocol.h"
#include "errors.h"
#include "netio.h"

#include <string.h>
#include <stdlib.h>

/* Write v into p[0..3], most significant byte first (big-endian). */
static void put_u32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

/* Write v into p[0..1], most significant byte first (big-endian). */
static void put_u16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
}

static uint32_t get_u32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | ((uint32_t)p[3]);
}

static uint16_t get_u16(const uint8_t *p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static void proto_pack(const msg_header_t *hdr, uint8_t out[PROTO_HEADER_SIZE]) {
    put_u32(out + 0, hdr->magic);
    put_u16(out + 4, hdr->version);
    put_u16(out + 6, hdr->type);
    put_u32(out + 8, hdr->payload_size);
    put_u32(out + 12, hdr->width);
    put_u32(out + 16, hdr->height);
    put_u32(out + 20, hdr->channels);
    put_u32(out + 24, hdr->crc32);
    put_u32(out + 28, hdr->server_time_us);
}

static void proto_unpack(const uint8_t in[PROTO_HEADER_SIZE], msg_header_t *hdr) {
    hdr->magic          = get_u32(in +  0);
    hdr->version        = get_u16(in +  4);
    hdr->type           = get_u16(in +  6);
    hdr->payload_size   = get_u32(in +  8);
    hdr->width          = get_u32(in + 12);
    hdr->height         = get_u32(in + 16);
    hdr->channels       = get_u32(in + 20);
    hdr->crc32          = get_u32(in + 24);
    hdr->server_time_us = get_u32(in + 28);
}

static const uint32_t crc32_divisor = 0xEDB88320u;

/* Run the 8 division steps on one byte value (bit by bit). */
static uint32_t crc32_byte(uint32_t value) {
    for (int k = 8; k; k--)
        value = (value & 1) ? (value >> 1) ^ crc32_divisor : value >> 1;
    return value;
}

uint32_t crc32_compute(const void *data, size_t len) {
    static uint32_t table[256];     /* lives for the whole program */
    static int ready = 0;           /* 0 until the table is built  */

    const uint8_t *buf = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFFu;

    /* first call only: build the table */
    if (!ready) {
        for (uint32_t n = 0; n < 256; n++)
            table[n] = crc32_byte(n);
        ready = 1;
    }

    /* one table lookup per byte */
    for (size_t i = 0; i < len; i++)
        crc = table[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);

    return crc ^ 0xFFFFFFFFu;
}

const char *proto_type_name(uint16_t type) {
    switch (type) {
    case MSG_TEXT:         return "TEXT";
    case MSG_IMAGE:        return "IMAGE";
    case MSG_TEXT_RESULT:  return "TEXT_RESULT";
    case MSG_IMAGE_RESULT: return "IMAGE_RESULT";
    case MSG_ERROR:        return "ERROR";
    default:               return "UNKNOWN";
    }
}

int proto_validate(const msg_header_t *hdr) {
    if(hdr == NULL) 
        return ERR_ARGS;
    
    if(hdr->magic != PROTO_MAGIC) 
        return ERR_PROTO_MAGIC;
    
    if(hdr->version != PROTO_VERSION) 
        return ERR_PROTO_VERSION;

    if(hdr->type < MSG_TEXT || hdr->type > MSG_ERROR)
        return ERR_PROTO_TYPE;
    
    if(hdr->payload_size > PROTO_MAX_PAYLOAD) 
        return ERR_PROTO_SIZE;
    
    
    if(hdr->type == MSG_IMAGE || hdr->type == MSG_IMAGE_RESULT) {
        if(hdr->channels != 1 && hdr->channels != 3) 
            return ERR_PROTO_SIZE;
        

        uint64_t pixels = (uint64_t)hdr->width * hdr->height;
        if(pixels > PROTO_MAX_PAYLOAD) 
            return ERR_PROTO_SIZE;
        
        if(pixels * hdr->channels != hdr->payload_size) 
            return ERR_PROTO_SIZE;
    } else if(hdr->type == MSG_TEXT || hdr->type == MSG_TEXT_RESULT) {
        if(hdr->width != 0 || hdr->height != 0 || hdr->channels != 0) 
            return ERR_PROTO_SIZE;
    }

    return ERR_OK;
}

int proto_send(int fd, const msg_header_t *info, const void *payload, uint32_t size) {

    if(info == NULL || (payload == NULL && size != 0))
        return ERR_ARGS;

    msg_header_t hdr = *info;
    
    hdr.magic = PROTO_MAGIC;
    hdr.version = PROTO_VERSION;
    hdr.payload_size = size;

    int rc = proto_validate(&hdr);
    if(rc != ERR_OK) {
        return rc;
    }

    hdr.crc32 = crc32_compute(payload, size);

    uint8_t header[PROTO_HEADER_SIZE];
    proto_pack(&hdr, header);

    rc = sendn(fd, header, PROTO_HEADER_SIZE);
    if(rc != ERR_OK) return rc;

    rc = sendn(fd, payload, hdr.payload_size);
    return rc;
}

int proto_recv(int fd, msg_header_t *hdr, uint8_t **payload) {
    if(hdr == NULL || payload == NULL) 
        return ERR_ARGS;
    
    *payload = NULL;
    uint8_t header[PROTO_HEADER_SIZE];
    int rc = recvn(fd, header, PROTO_HEADER_SIZE);

    if(rc != ERR_OK) return rc;

    proto_unpack(header, hdr);
    rc = proto_validate(hdr);

    if(rc != ERR_OK) 
        return rc;

    uint8_t* buf = malloc(hdr->payload_size + 1);
    if(buf == NULL) 
        return ERR_NOMEM;

    rc = recvn(fd, buf, hdr->payload_size);

    if(rc != ERR_OK) {
        free(buf);
        return rc;
    }

    uint32_t crc = crc32_compute(buf, hdr->payload_size);

    if(crc != hdr->crc32) {
        free(buf);
        return ERR_CRC;
    }
    buf[hdr->payload_size] = '\0';

    *payload = buf;
    return ERR_OK;
}