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

#define MAXDATASIZE 10 // max number of bytes we can get at once
#define MAXLINE 1024
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

ssize_t readline(int fd, void *vptr, size_t maxlen) {
    ssize_t n, rc;
    char c, *ptr;
    ptr = vptr;
    for (n = 1; n < maxlen; n++) {
        again:
        if ((rc = read(fd, &c, 1)) == 1) {
            *ptr++ = c;
            if (c == '\n') {
                break; //detect a newline
            }
        } else if (rc == 0) {
            *ptr = 0;
            return (n - 1); // detect EOF, n - 1 bytes were read
        } else {
            if (errno == EINTR) goto again;
            return (-1); /* error, errno set by read() */
        }
    }
    *ptr = 0; /* null terminate like fgets() */
    return (n);
}

ssize_t writen(int fd, const void *vptr, size_t n) {
    size_t nleft;
    ssize_t nwritten;
    const char *ptr;
    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
            if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0; //Detect the 'Enter' error and call write() again
            else
                return (-1); /* error */
            }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return (n);
}

void str_cli(FILE *fp, int sockfd){
	char sendline[MAXLINE], recvline[MAXLINE];
	while (fgets(sendline, MAXLINE, fp) != NULL) {
      if (strlen (sendline) > MAXDATASIZE + 1 ) {
          printf("Your input exceeds the MAXDATASIZE!\n");
          continue;
      }
	  writen(sockfd, sendline, strlen (sendline));
	  if (readline(sockfd, recvline, MAXLINE) == 0) {
          perror("str_cli: server terminated prematurely");
      }
	  fputs(recvline, stdout);
    }
}

int main(int argc, char *argv[]) {
    char* PORT = argv[2];
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv, num;
    char s[INET6_ADDRSTRLEN];
    if (argc != 3) {
        fprintf(stderr,"usage: client hostname, PORT\n");
        exit(1);
    }
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
            p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s, sizeof s);
    printf("client: connecting to %s\n", s);
    freeaddrinfo(servinfo); // all done with this structure


    str_cli(stdin, sockfd);
    close(sockfd);
    return 0;
}
