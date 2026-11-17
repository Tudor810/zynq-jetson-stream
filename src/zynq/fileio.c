#include "fileio.h"
#include "common/errors.h"

#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

int file_read_all(const char *path, uint8_t **data, size_t *size)
{   
    if (path == NULL || data == NULL || size == NULL)
        return ERR_ARGS;

    uint8_t *buf = NULL;
    size_t total = 0;
    size_t got = 0;
    struct stat statbuf;
    *data = NULL;
    *size = 0;
    int err = ERR_OK;
    int fd = open(path, O_RDONLY);
    
    if(fd < 0) {
        err = ERR_FILE_OPEN;
        goto cleanup;
    }
    
    if(fstat(fd, &statbuf) == -1) {
        err = ERR_FILE_OPEN;
        goto cleanup;
    }

    if(!S_ISREG(statbuf.st_mode)) {
        err = ERR_FILE_READ;
        goto cleanup;
    }
    
    total = (size_t)statbuf.st_size;
    buf = (uint8_t *)malloc(total + 1);

    if(buf == NULL) {
        err = ERR_NOMEM;
        goto cleanup;
    } 

    while(got < total) {
        ssize_t n = read(fd, buf + got, total - got);
        if(n < 0) {
            if(errno == EINTR) 
                continue;
            err = ERR_FILE_READ;
            goto cleanup;
        }

        if(n == 0) 
            break;
        got += (size_t)n;
    }

    if(got != total) {
        err = ERR_FILE_READ;
        goto cleanup;
    }

    buf[total] = '\0';
    *data = buf;
    *size = total;
    buf = NULL;

    cleanup: 
    if(fd >= 0) close(fd);
    if(buf)     free(buf);

    return err;
}   

int file_write_all(const char *path, const void *data, size_t size)
{
    if (path == NULL || (size > 0 && data == NULL))
        return ERR_ARGS;

    const uint8_t *p = (const uint8_t *)data;
    int err = ERR_OK;
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if(fd < 0) {
        err = ERR_FILE_OPEN;
        goto cleanup;
    }

    size_t got = 0;

    while(got < size) {
        ssize_t n = write(fd, p + got, size - got);
        if(n < 0) {
            if(errno == EINTR) 
                continue;
            err = ERR_FILE_WRITE;
            goto cleanup;
        }

        if(n == 0) 
            break;
        got += (size_t)n;
    }

    if(got != size) {
        err = ERR_FILE_WRITE;
        goto cleanup;
    }

    if(fsync(fd) < 0) {
        err = ERR_FILE_WRITE;
        goto cleanup;
    }

    if(close(fd) < 0) 
        err = ERR_FILE_WRITE;
    fd = -1;

cleanup:
    if(fd >= 0) 
        close(fd);
    return err;
}
