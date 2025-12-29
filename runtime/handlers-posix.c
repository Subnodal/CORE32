#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <poll.h>

#include "handlers-posix.h"
#include "handlers.h"

#define FILE_READ       0x01
#define FILE_WRITE      0x02
#define FILE_READ_WRITE 0x03
#define FILE_APPEND     0x04
#define FILE_CREATE     0x08
#define FILE_TRUNCATE   0x10

void c32_fileOpen(CORE32* vm) {
    c32_Long mode = c32_pop(vm, C32_FMT_LONG);
    c32_Byte* path = c32_safeGetBytes(vm, c32_pop(vm, C32_FMT_LONG));
    c32_Long translatedMode = 0;

    if ((mode & FILE_READ) && (mode & FILE_WRITE)) {
        translatedMode |= O_RDWR;
    } else {
        if (mode & FILE_READ) translatedMode |= O_RDONLY;
        if (mode & FILE_WRITE) translatedMode |= O_WRONLY;
    }

    if (mode & FILE_APPEND) translatedMode |= O_APPEND;
    if (mode & FILE_CREATE) translatedMode |= O_CREAT;
    if (mode & FILE_TRUNCATE) translatedMode |= O_TRUNC;

    long fd = open(path, translatedMode, S_IRUSR | S_IWUSR);

    if (fd < 0) {
        c32_push(vm, C32_FMT_LONG, 0);
        c32_push(vm, C32_FMT_BYTE, 0);

        return;
    }

    c32_push(vm, C32_FMT_LONG, fd);
    c32_push(vm, C32_FMT_BYTE, 1);
}

void c32_fileClose(CORE32* vm) {
    c32_Long fd = c32_pop(vm, C32_FMT_LONG);

    close(fd);
}

void c32_fileSize(CORE32* vm) {
    c32_Long fd = c32_pop(vm, C32_FMT_LONG);

    struct stat stats;

    if (fstat(fd, &stats)) {
        c32_push(vm, C32_FMT_LONG, 0);
        return;
    }

    c32_push(vm, C32_FMT_LONG, stats.st_size);
}

void c32_fileSeek(CORE32* vm) {
    c32_Long fd = c32_pop(vm, C32_FMT_LONG);
    c32_Long pos = c32_pop(vm, C32_FMT_LONG);

    c32_push(vm, C32_FMT_BYTE, lseek(fd, pos, SEEK_SET) < 0 ? 0 : 1);
}

void c32_fileTell(CORE32* vm) {
    c32_Long fd = c32_pop(vm, C32_FMT_LONG);

    long pos = lseek(fd, 0, SEEK_CUR);
    
    c32_push(vm, C32_FMT_LONG, pos < 0 ? 0 : pos);
}

void c32_fileAvailable(CORE32* vm) {
    c32_Long fd = c32_pop(vm, C32_FMT_LONG);

    struct pollfd pfd = {
        .fd = fd,
        .events = POLLIN
    };

    if (!poll(&pfd, 1, 0)) {
        c32_push(vm, C32_FMT_BYTE, 0);
        return;
    }

    c32_push(vm, C32_FMT_BYTE, (pfd.revents & POLLIN) ? 1 : 0);
}

void c32_fileReadChar(CORE32* vm) {
    c32_Long fd = c32_pop(vm, C32_FMT_LONG);
    char c[1] = {0};
    int result = read(fd, c, 1);

    if (result <= 0) {
        c32_push(vm, C32_FMT_BYTE, result == 0 ? 0x04 : 0); // EOT char sent on EOF
        c32_push(vm, C32_FMT_BYTE, 0);
        return;
    }

    c32_push(vm, C32_FMT_BYTE, c[0]);
    c32_push(vm, C32_FMT_BYTE, 1);
}

void c32_fileWriteChar(CORE32* vm) {
    c32_Long fd = c32_pop(vm, C32_FMT_LONG);
    char c[1] = {c32_pop(vm, C32_FMT_BYTE)};

    c32_push(vm, C32_FMT_BYTE, write(fd, c, 1) == 1);
}

void c32_registerPosixHandlers(CORE32* vm) {
    c32_registerHandler(vm, c32_id("fOPN"), c32_fileOpen);
    c32_registerHandler(vm, c32_id("fCLS"), c32_fileClose);
    c32_registerHandler(vm, c32_id("fSIZ"), c32_fileSize);
    c32_registerHandler(vm, c32_id("fSEK"), c32_fileSeek);
    c32_registerHandler(vm, c32_id("fTEL"), c32_fileTell);
    c32_registerHandler(vm, c32_id("fAVL"), c32_fileTell);
    c32_registerHandler(vm, c32_id("fRDC"), c32_fileReadChar);
    c32_registerHandler(vm, c32_id("fWRC"), c32_fileWriteChar);
}