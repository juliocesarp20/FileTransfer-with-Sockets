#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include "common.h"

#define BUFSZ 500
#define SIZEOPTION 12
#define MAXEXTENSIONLENGTH 4

/**
 * Prints the usage of the server program
 *
 * - argc: The number of command-line arguments
 * - argv: An array of command-line argument strings
 */

void serverUsage(int argc, char **argv)
{
    printf("usage: %s <v4 or v6> <server port>\n", argv[0]);
    exit(EXIT_FAILURE);
}

/**
 * Checks if the given string ends with "\\end"
 *
 * - str: The input string to check
 *
 * Returns:
 *  1 if the string ends with "\\end", 0 otherwise
 */
int endsWithEnd(const char *str)
{
    const char *suffix = "\\end";
    size_t strLength = strlen(str);
    size_t suffixLength = strlen(suffix);

    if (suffixLength > strLength)
    {
        return 0;
    }

    const char *strSuffix = str + (strLength - suffixLength);

    return strcmp(strSuffix, suffix) == 0;
}

/**
 * Removes the "\\end" from the given string
 *
 * - str: The input string to modify
 */

void removeEnd(char *str)
{
    char *endPtr = strstr(str, "\\end");
    if (endPtr != NULL)
    {
        *endPtr = '\0';
    }
}

/**
 * Removes the name from the content
 *
 * - name: The name to remove
 * - content: The content string to modify
 */

int removeName(char *name, char *content)
{
    char *match = strstr(content, name);

    if (match != NULL)
    {
        size_t nameLen = strlen(name);
        memmove(match, match + nameLen, strlen(match + nameLen) + 1);
        return 1;
    }

    return 0;
}


/**
 * Gets the option bytes from the client
 *
 * - csock: The client socket
 * - buf: The buffer to store the received data
 * - bufferSize: The size of the buffer
 *
 * Returns:
 *  - the option enum if successful, or CONNECTION_CLOSED if the connection was closed
 */
enum Options getOptionBytes(int csock, char *buf)
{
    int bytesReceived;
    int option = 0;

    bytesReceived = recv(csock, buf, BUFSZ, 0);

    if (bytesReceived == -1 || bytesReceived == 0)
        return CONNECTION_CLOSED;

    if (endsWithEnd(buf)){
        buf[bytesReceived] = '\0';
        return SEND;
    }

    if (strncmp(buf, "exit",4) == 0){
        buf[bytesReceived-1] = '\0';
        return EXIT;
    }
    
    return INVALID_OPERATION;

}

int main(int argc, char *argv[])
{
    // Check the number of command-line arguments
    if (argc < 3)
    {
        serverUsage(argc, argv);
    }

    // Initialize the socket address storage
    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage))
    {
        serverUsage(argc, argv);
    }

    // Create a socket
    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1)
    {
        exit(EXIT_FAILURE);
    }

    // Enable address reuse
    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)))
    {
        exit(EXIT_FAILURE);
    }

    // Convert the sockaddr_storage to sockaddr
    struct sockaddr *addr = (struct sockaddr *)(&storage);
    // Bind the socket to the address
    if (0 != bind(s, addr, sizeof(storage)))
    {
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (0 != listen(s, 10))
    {
        exit(EXIT_FAILURE);
    }

    char addrstr[BUFSZ];
    // Convert the socket address to a string representation
    addrtostr(addr, addrstr, BUFSZ);

    printf("Server on %s, waiting\n", addrstr);

    while (1)
    {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        // Accept a new client connection
        int csock = accept(s, caddr, &caddrlen);

        if (csock == -1)
        {
            exit(EXIT_FAILURE);
        }

        char caddrstr[BUFSZ];
        // Convert the client socket address to a string representation
        addrtostr(caddr, caddrstr, BUFSZ);

        char buf[BUFSZ];
        memset(buf, 0, BUFSZ);
        size_t count;
        char fileNameExtracted[BUFSZ];
        int isReceivingFile = 0;
        int overwrite = 0;
        int option = -1;
        FILE *file = NULL;
        memset(fileNameExtracted, 0, BUFSZ);

        while (1)
        {
            memset(buf, 0, BUFSZ);
            option = getOptionBytes(csock, buf);

            // Checks if there was a connection error while receiving bytes
            if (option == CONNECTION_CLOSED)
            {
                if (isReceivingFile)
                {
                    extractFileNameExtension(buf, fileNameExtracted);
                    printf("error receiving file %s\n", fileNameExtracted);
                }
                if (file != NULL)
                {
                    fclose(file);
                    file = NULL;
                }
                break;
            }

            puts(buf);

            if (option == EXIT)
            {
                memset(buf, 0, BUFSZ);
                printf("connection closed\n");
                break;
            }

            if (option==SEND)
            {
                extractFileNameExtension(buf, fileNameExtracted);
                if (strlen(fileNameExtracted) != 0)
                {
                    if (fileExists(fileNameExtracted))
                        overwrite = 1;
                    else
                        overwrite = 0;
                    if (removeName(fileNameExtracted, buf) == 0)
                    {
                        printf("error receiving file %s\n", fileNameExtracted);
                        break;
                    }

                    removeEnd(buf);

                    file = fopen(fileNameExtracted, "w");

                    if (file != NULL)
                        fprintf(file, "%s", buf);

                    if (overwrite)
                    {
                        char message[500];
                        printf("file %s overwritten\n", fileNameExtracted);
                        strcpy(message, "file ");
                        strcat(message, fileNameExtracted);
                        strcat(message, " overwritten");
                        int count = send(csock, message, strlen(message), 0);

                        if (count != strlen(message))
                        {
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        printf("file %s received\n", fileNameExtracted);
                        char message[500];
                        strcpy(message, "file ");
                        strcat(message, fileNameExtracted);
                        strcat(message, " received");
                        int count = send(csock, message, strlen(message), 0);
                    }

                    if (file != NULL)
                    {
                        fclose(file);
                        file = NULL;
                    }
                }
            }

            memset(buf, 0, BUFSZ);
        }
        if (file != NULL)
        {
            fclose(file);
            file = NULL;
        }
        close(csock);
    }
    exit(EXIT_SUCCESS);
}