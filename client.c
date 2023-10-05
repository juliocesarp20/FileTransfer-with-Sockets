#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

// socket libraries:
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define SIZEOPTION 12
#define BUFSZ 500

void clientUsage(int argc, char **argv)
{
    printf("usage: %s <server IP> <server port>\n", argv[0]);
    exit(EXIT_FAILURE);
}

/*
 * Remove special characters, except line breaks and spaces
 */
int isSpecialCharacter(char c)
{
    if (isalnum(c) || c == ' ' || c == '\n')
    {
        return 0;
    }
    return 1;
}

void removeSpecialCharacters(char *str)
{
    int i, j;
    for (i = 0, j = 0; str[i] != '\0'; i++)
    {
        if (!isSpecialCharacter(str[i]))
        {
            str[j++] = str[i];
        }
    }
    str[j] = '\0';
}

/*
 * Send a message through the socket.
 * Parameters:
 *   s: socket
 *   buf: buffer containing the message
 */

/*
 * Send a file through the socket.
 * Parameters:
 *   - buf: buffer to store the data to be sent
 *   - fileNameCount: length of the file name extracted from the command
 *   - fileNameExtracted: extracted file name from the command
 *   - fp: file pointer of the file to be sent
 *   - s: socket descriptor
 * Returns:
 *   - 0 if the file is sent successfully
 *   - exits if there was an error
 */

int sendFile(char *buf, int fileNameCount, const char *fileNameExtracted, FILE *fp, int s)
{
    int i = 0;
    char buffer[BUFSZ];
    char finalBuffer[BUFSZ];
    int readCount = 0;
    int maxSize = BUFSZ - fileNameCount - strlen("\\end");

    int bytesRead = fread(buffer, sizeof(char), maxSize, fp);

    removeSpecialCharacters(buffer);

    strcpy(finalBuffer, fileNameExtracted);

    strcat(finalBuffer, buffer);

    strcat(finalBuffer, "\\end");

    strcpy(buf, finalBuffer);

    int count = send(s, buf, strlen(buf), 0);

    if (count != strlen(buf))
    {
        exit(EXIT_FAILURE);
    }

    memset(buf, 0, BUFSZ);

    return 0;
}

/*
 * Get the client option based on the user's input.
 * Parameters:
 *   - option: user's input option string
 * Returns:
 *   - The corresponding option enum value.
 */

enum Options getClientOptions(const char *option)
{
    char temp[BUFSIZ];
    strncpy(temp, option, sizeof(temp));
    temp[sizeof(temp) - 1] = '\0';

    size_t length = strcspn(temp, "\n");
    temp[length] = '\0';

    if (strcmp(temp, "exit") == 0)
        return EXIT;

    if (strcmp(temp, "send file") == 0)
        return SEND;

    if (strncmp(temp, "select file ", SIZEOPTION) == 0)
    {

        const char *fileName = temp + SIZEOPTION;

        if (!fileExists(fileName))
            return SELECT_NOT_EXISTS;

        if (fileIsValidType(fileName))
            return SELECT_VALID;

        return SELECT_INVALID;
    }

    return INVALID_OPERATION;
}

int main(int argc, char **argv)
{
    // Check if the required command-line arguments are provided
    if (argc < 3)
    {
        clientUsage(argc, argv);
    }

    // Parse the server IP address and port number
    struct sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], &storage))
    {
        clientUsage(argc, argv);
        exit(EXIT_FAILURE);
    }

    // Create a socket
    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1)
    {
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != connect(s, addr, sizeof(storage)))
    {
        exit(EXIT_FAILURE);
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);

    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);

    size_t count;
    int fileSelected = 0;
    int shouldSendFile = 0;
    int sendEnd = 0;
    char fileNameExtracted[BUFSZ];
    FILE *fp;
    int fileNameCount = 0;

    while (1)
    {
        // Read user input
        fgets(buf, BUFSZ - 1, stdin);

        int option = (int)getClientOptions(buf);

        switch (option)
        {
        case SEND:
            // Check if a file is selected before sending
            if (fileSelected == 0)
            {
                puts("no file selected!");
            }
            else
            {
                // Open the file to be sent
                fp = fopen(fileNameExtracted, "rb");

                // Send the file
                sendEnd = sendFile(buf, fileNameCount, fileNameExtracted, fp, s);

                count = recv(s, buf, BUFSZ, 0);
                if (count == -1)
                {
                    exit(EXIT_FAILURE);
                }

                puts(buf);

                fclose(fp);
            }
            break;
        case SELECT_NOT_EXISTS:
            // Extract the file name and notify that it doesn't exist
            extractFileName(buf, fileNameExtracted);
            printf("%s do not exist\n", fileNameExtracted);
            fileSelected = 0;
            break;
        case SELECT_INVALID:
            // Extract the file name and notify that it is not valid
            extractFileName(buf, fileNameExtracted);
            printf("%s not valid!\n", fileNameExtracted);
            fileSelected = 0;
            break;
        case SELECT_VALID:
            // Extract the file name and notify that it is selected
            extractFileName(buf, fileNameExtracted);
            fileNameCount = strlen(fileNameExtracted);
            printf("%s selected\n", fileNameExtracted);
            fileSelected = 1;
            break;
        case EXIT:
            // Send the exit command to the server and close the socket
            count = send(s, buf, strlen(buf), 0);
            close(s);
            exit(EXIT_SUCCESS);
            break;
        case INVALID_OPERATION:
            close(s);
            exit(EXIT_FAILURE);
            break;
        }
    }

    // Close the socket
    close(s);

    exit(EXIT_SUCCESS);
}