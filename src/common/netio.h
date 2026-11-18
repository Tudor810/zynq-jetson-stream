/*
 * netio.h - reliable byte transfer over a connected TCP socket.
 *
 * TCP is a byte stream: one send() may arrive as several recv() calls.
 * sendn/recvn loop until exactly n bytes have moved.
 * send_message/recv_message add a 4-byte length in front of the payload
 * (temporary: replaced by the full header in protocol.h later).
 */
#ifndef NETIO_H
#define NETIO_H

#include <stddef.h>
#include <stdint.h>


/* Largest payload accepted by recv_message (protects against bad lengths). */
#define NETIO_MAX_PAYLOAD (64u * 1024u * 1024u)

/* Default server port - lives in common so that both client and server knows about it*/
#define DEFAULT_PORT 5000

/*
 * Send exactly n bytes from buf.
 * Returns ERR_OK or ERR_SEND (see errno).
 */
int sendn(int fd, const void *buf, size_t n);

/*
 * Receive exactly n bytes into buf.
 * Returns ERR_OK, ERR_CLOSED (peer closed the connection) or ERR_RECV.
 */
int recvn(int fd, void *buf, size_t n);

/*
 * Send one message: 4-byte length (network byte order), then the payload.
 * Returns ERR_OK, ERR_ARGS or ERR_SEND.
 */
int send_message(int fd, const void *payload, uint32_t len);

/*
 * Receive one message sent with send_message.
 * On success *out is a malloc'ed buffer (caller frees) and *len its size.
 * Returns ERR_OK, ERR_CLOSED, ERR_RECV, ERR_NOMEM or ERR_PROTO_SIZE.
 */
int recv_message(int fd, void **out, uint32_t *len);

#endif /* NETIO_H */