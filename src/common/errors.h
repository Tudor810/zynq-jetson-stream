/*
 * errors.h - error codes for the whole project.
 *
 * Convention: functions return ERR_OK (0) on success or a negative ERR_*
 * code on failure. Results come back through pointer arguments.
 * Add new codes here as you need them (network, protocol, CRC...).
 */
#ifndef ERRORS_H
#define ERRORS_H

typedef enum {
    ERR_OK            =  0,
    ERR_ARGS          = -1,   /* bad arguments passed to a function                        */
    ERR_NOMEM         = -2,   /* malloc failed                                             */
    ERR_FILE_OPEN     = -3,   /* open() failed, see errno                                  */
    ERR_FILE_READ     = -4,   /* read() failed or file too short                           */
    ERR_FILE_WRITE    = -5,   /* write() / close() failed                                  */
    ERR_SOCKET        = -6,   /* socket(), setsockopt(), bind(), listen(), accept() failed */
    ERR_ADDRESS       = -7,   /* IP address text could not be parsed (inet_pton)           */
    ERR_CONNECT       = -8,   /* connect() failed (server not running?)                    */
    ERR_SEND          = -9,   /* send() failed                                             */
    ERR_RECV          = -10,  /* recv() failed                                             */
    ERR_CLOSED        = -11,  /* the other side closed the connection                      */
    ERR_PROTO_SIZE    = -12,  /* message length is 0 or above the limit                    */
    ERR_FORMAT        = -13,  /* file is not a valid PPM/PGM                               */
    ERR_UNSUPPORTED   = -14,  /* valid PPM/PGM, but a variant not handled                  */
    ERR_PROTO_MAGIC   = -15,  /* header does not start with PROTO_MAGIC                    */
    ERR_PROTO_VERSION = -16,  /* unknown protocol version                                  */
    ERR_PROTO_TYPE    = -17,  /* unknown or unexpected message type                        */
    ERR_CRC           = -18,  /* payload CRC32 does not match the header                   */
    ERR_REMOTE        = -19   /* the other side replied with MSG_ERROR                     */
} err_t;

/* Short text for an error code. */
const char *err_str(int code);

/* Print "context: error text (errno text)" to stderr. */
void err_print(const char *context, int code);

#endif /* ERRORS_H */
