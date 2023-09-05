/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "9000" // the port client will be connecting to

#define MAXDATASIZE 4096 // max number of bytes we can get at once

void slice(const char *str, char *buffer, size_t start, size_t end)
{
    size_t j = 0;
    for (size_t i = start; i <= end; ++i)
    {
        buffer[j++] = str[i];
    }
    buffer[j] = 0;
}
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv, n;
    char s[INET6_ADDRSTRLEN];
    char request[4096];
    char filename[2000];
    char host[2000];
    char ip[2000];
    char port[] = "80";
    size_t split;
    FILE *fp = NULL;
    if (argc != 2)
    {
        fprintf(stderr, "missing arguments, usage: client hostname\n");
        exit(1);
    }
    // get file name, ip, port.
    for (split = 7; split < strlen(argv[1]); split++)
    {
        if (argv[1][split] == '/')
            break;
    }
    slice(argv[1], filename, split + 1, strlen(argv[1]) - 1);
    slice(argv[1], host, 7, split - 1);
    split = 0;
    for (int i = 7; i < strlen(host); i++)
    {
        if (host[i] == ':')
        {
            split = i;
            break;
        }
    }
    if (split > 0)
    {
        slice(host, port, split + 1, strlen(host) - 1);
        slice(host, ip, 0, split - 1);
    }
    else
    {
        slice(host, ip, 0, strlen(host) - 1);
    }
    printf("%s\n%s\n", ip, filename);
    printf("%s\n", port);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if ((rv = getaddrinfo(ip, port, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    snprintf(request, 4096, "GET /%s HTTP/1.1\r\nHost: %s:%s\r\n\r\n", filename, ip, port);
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
              s, sizeof s);
    printf("http_client: connecting to %s, port %s\n", s, port);
    printf("http_client: sending %s\n", request);

    n = write(sockfd, request, strlen(request));
    if (n < 0)
    {
        perror("ERROR writing to socket");
        exit(0);
    }
    freeaddrinfo(servinfo); // all done with this structure
    fp = fopen("output", "w");
    int total_bytes = 0;
    while ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) > 0)
    {

        char *content = strstr(buf, "\r\n\r\n"); // remove header.
        if (content != NULL)
        {
            content += 4; // Offset by 4 bytes to start of content
        }
        else
        {
            content = buf;
        }

        fprintf(fp, "%s", content);
        bzero(buf, MAXDATASIZE - 1);
        total_bytes += numbytes;
    }
    if ((numbytes) == -1)
    {
        perror("recv");
        exit(1);
    }
    buf[numbytes] = '\0';

    printf("client: received %d bytes in total.\n", total_bytes);

    fclose(fp);
    close(sockfd);

    return 0;
}
