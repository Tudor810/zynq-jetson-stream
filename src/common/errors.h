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
    ERR_OK         =  0,
    ERR_ARGS       = -1,   /* bad arguments passed to a function   */
    ERR_NOMEM      = -2,   /* malloc failed                        */
    ERR_FILE_OPEN  = -3,   /* open() failed, see errno             */
    ERR_FILE_READ  = -4,   /* read() failed or file too short      */
    ERR_FILE_WRITE = -5    /* write() / close() failed             */
} err_t;

/* Short text for an error code. */
const char *err_str(int code);

/* Print "context: error text (errno text)" to stderr. */
void err_print(const char *context, int code);

#endif /* ERRORS_H */
