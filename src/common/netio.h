/*
 * netio.h - reliable byte transfer over a connected TCP socket.
 *
 * TCP is a byte stream: one send() may arrive as several recv() calls.
 * sendn/recvn loop until exactly n bytes have moved.
 */
#ifndef NETIO_H
#define NETIO_H

#include <stddef.h>
#include <stdint.h>

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


#endif /* NETIO_H */