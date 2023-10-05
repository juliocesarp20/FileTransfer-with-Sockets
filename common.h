#ifndef COMMON_H
#define COMMON_H
#pragma once

#include <stdlib.h>

#include <arpa/inet.h>

// Supported file types
enum FileType
{
    TXT,
    C,
    CPP,
    PY,
    TEX,
    JAVA
};

// Supported options
enum Options
{
    SEND = 1,
    SELECT_VALID = 2,
    SELECT_NOT_EXISTS = 3,
    SELECT_INVALID = 4,
    EXIT = 5,
    CONNECTION_CLOSED = 6,
    INVALID_OPERATION = -1
};

int addrparse(const char *addrstr, const char *portstr,
              struct sockaddr_storage *storage);

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);

int server_sockaddr_init(const char *proto, const char *portstr,
                         struct sockaddr_storage *storage);

int fileIsValidType(const char *filename);

int fileExists(const char *filename);

void extractFileName(const char *option, char *fileName);

int endsWithEnd(const char *str);

void removeEnd(char *str);

void extractFileNameExtension(const char *string, char *filename);

#endif