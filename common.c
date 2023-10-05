#include "common.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

static const char *extensions[] = {"java", "txt", "tex", "cpp", "py", "c"};
static const int num_extensions = sizeof(extensions) / sizeof(const char *);
#define SIZEOPTION 12
#define MAXEXTENSIONLENGTH 4

/*
 * Extracts the file name and the extension of the file. 
 * - string is the input
 * - filename is the result
 */
void extractFileNameExtension(const char *string, char *filename)
{
    size_t longest_length = MAXEXTENSIONLENGTH;

    const char *dot = strchr(string, '.');
    if (dot != NULL)
    {
        size_t filename_length = dot - string;
        strncpy(filename, string, filename_length);
        filename[filename_length] = '\0';

        const char *extension = dot + 1;

        for (size_t i = 0; i < longest_length; i++)
        {
            char current_char = extension[i];
            if (current_char == '\0')
            {
                break;
            }

            // Check if the current extension matches any of the valid extensions
            for (int j = 0; j < num_extensions; j++)
            {
                const char *valid_extension = extensions[j];
                if (strncmp(extension + i, valid_extension, strlen(valid_extension)) == 0)
                {
                    strcat(filename, ".");
                    strcat(filename, extensions[j]);
                    return;
                }
            }
        }
    }
    else
    {
        strcpy(filename, string);
    }
}

/*
 * Check if the file name has a valid extension.
 * Returns 1 if valid, 0 otherwise.
 */
int fileIsValidType(const char *filename)
{
    const char *extension = strrchr(filename, '.');
    if (extension == NULL)
    {
        return 0;
    }

    extension++;

    for (int i = 0; i < num_extensions; i++)
    {
        if (strcasecmp(extension, extensions[i]) == 0)
        {
            return 1;
        }
    }

    return 0;
}

/*
 * Check if the file with the given name exists.
 * Returns 1 if the file exists, 0 otherwise.
 */
int fileExists(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file != NULL)
    {
        fclose(file);
        return 1;
    }
    return 0;
}

/*
 * Extract the file name from the input string.
 * The input string is expected to be in the format "select file [filename]".
 * The extracted name is stored in the 'fileName' parameter.
 */
void extractFileName(const char *option, char *fileName)
{
    const char *start = option + SIZEOPTION;

    size_t length = strcspn(start, "\n");

    strncpy(fileName, start, length);
    fileName[length] = '\0';
}

/*
 * Parse the address and port strings and initialize sockaddr.
 * Returns 0 on success, -1 on failure.
 */
int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage)
{
    if (addrstr == NULL || portstr == NULL)
    {
        exit(EXIT_FAILURE);
    }

    uint16_t port = (uint16_t)atoi(portstr);

    if (port == 0)
    {
        return -1;
    }
    port = htons(port);

    struct in_addr inaddr4;
    if (inet_pton(AF_INET, addrstr, &inaddr4))
    {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;
        return 0;
    }

    struct in6_addr inaddr6;
    if (inet_pton(AF_INET6, addrstr, &inaddr6))
    {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        memcpy(&(addr6->sin6_addr), &inaddr6, sizeof(inaddr6));
        return 0;
    }

    return -1;
}

/*
 * Convert the sockaddr to a string.
 * The result is stored in the 'str' parameter.
 */
void addrtostr(const struct sockaddr *addr, char *str, size_t strsize)
{
    int version;
    char addrstr[INET6_ADDRSTRLEN + 1] = "";
    uint16_t port;

    if (addr->sa_family == AF_INET)
    {
        version = 4;
        struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
        if (!inet_ntop(AF_INET, &(addr4->sin_addr), addrstr, INET6_ADDRSTRLEN + 1))
        {
            exit(EXIT_FAILURE);
        }
        port = ntohs(addr4->sin_port);
    }
    else if (addr->sa_family == AF_INET6)
    {
        version = 6;
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;
        if (!inet_ntop(AF_INET6, &(addr6->sin6_addr), addrstr, INET6_ADDRSTRLEN + 1))
        {
            exit(EXIT_FAILURE);
        }
        port = ntohs(addr6->sin6_port);
    }
    else
    {
        exit(EXIT_FAILURE);
    }

    if (str)
    {
        snprintf(str, strsize, "IPv%d %s %hu", version, addrstr, port);
    }
}

/*
 * Initialize the sockaddr_storage structure.
 * The 'proto' parameter specifies the protocol ("v4" or "v6").
 * The 'portstr' parameter specifies the port number.
 * Returns 0 on success, -1 on failure.
 */
int server_sockaddr_init(const char *proto, const char *portstr, struct sockaddr_storage *storage)
{
    uint16_t port = (uint16_t)atoi(portstr);
    if (port == 0)
    {
        return -1;
    }
    port = htons(port);

    memset(storage, 0, sizeof(*storage));

    if (0 == strcmp(proto, "v4"))
    {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_addr.s_addr = INADDR_ANY;
        addr4->sin_port = port;
        return 0;
    }
    else if (0 == strcmp(proto, "v6"))
    {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_addr = in6addr_any;
        addr6->sin6_port = port;
        return 0;
    }
    else
    {
        return -1;
    }
}