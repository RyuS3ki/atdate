/*
 * EchoClient.c
 * Aplicaciones Telematicas - Grado en Ingeneria Telematica 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <signal.h>

#define BUFSIZE 1024

int sockfd;

void sigint_handler(int signo)
{
	/* things to do after a Ctrl-C, before finishing the program*/
	puts("SIGINT received...");
	exit(0);
}


int main(int argc, char *argv[])
{
	struct hostent *server;
	struct sockaddr_in serveraddr;
	char* host;
	int port;
        char buf[BUFSIZE+1];
        int n;
        int outchars, inchars;

        /* handler SIGINT signal*/
	signal(SIGINT, sigint_handler);

        /* check command line arguments */
	switch (argc) {
	case 3:
		host = argv[1];
		port = atoi(argv[2]);
		break;
	default:
		fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
		exit(1);
        }

        /* socket: create the socket */
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
                perror("ERROR opening socket");
                exit(0);
        }

        /* gethostbyname: get the server's DNS entry */
        server = gethostbyname(host);
        if (server == NULL) {
                fprintf(stderr,"ERROR, no such host: %s\n", host);
                exit(0);
        }

        /* build the server's Internet address */
        bzero((char *) &serveraddr, sizeof(serveraddr));
        serveraddr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr, server->h_length);
        serveraddr.sin_port = htons(port);

        /* connect: create a connection with the server */
        if (connect(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
                perror("ERROR connecting");
                exit(0);
        }

        /* get message line from the user */
        while (fgets(buf, BUFSIZE, stdin)) {
                /* send the message line to the server */
                buf[BUFSIZE] = '\0';    /* insure line null-terminated  */
                outchars = strlen(buf);
                n = send(sockfd, buf, outchars,0);
                if (n < 0) {
                        perror("ERROR writing to socket");
                        exit(0);
                }

		/* read answer from the server */
                for (inchars = 0; inchars < outchars; inchars+=n ) {
                        n = recv(sockfd, &buf[inchars], outchars - inchars, 0);
                        if (n < 0) {
                                perror("ERROR reading from socket");
                                exit(0);
                        }
                }
                /* print the server's reply */
                fputs(buf, stdout);
        }
        close(sockfd);
	return(0);
}
