/*
 * fileio.h - read/write whole files (the SD card is a normal filesystem).
 */
#ifndef FILEIO_H
#define FILEIO_H

#include <stddef.h>
#include <stdint.h>

/*
 * Read the whole file at path into a new malloc'ed buffer.
 * On success: *data = buffer (caller frees), *size = number of bytes.
 * Returns ERR_OK or ERR_* from errors.h.
 */
int file_read_all(const char *path, uint8_t **data, size_t *size);

/* Create or overwrite path with size bytes from data. */
int file_write_all(const char *path, const void *data, size_t size);

#endif /* FILEIO_H */
