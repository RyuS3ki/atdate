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
#define DEF_PORT 37

int sockfd;

void usage(){
	fprintf(stderr, "usage: atdate [-h serverhost] [-p port] [-m cu|ct|s] [-d]\n");
	exit(1);
}

void sigint_handler(int signo){
	/* things to do after a Ctrl-C, before finishing the program*/
	puts("SIGINT received...");
	exit(0);
}

int main(int argc, char *argv[]){
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
	switch(argc){
		case 3:
			// MODO SERVIDOR
			if(argv[1] == "-m" && argv[2] == "s"){
				// Llamar al modo servidor
			}else if(argv[1] == "-h"){ // MODO CLIENTE
				// Modo UDP (default)
				host = argv[2];
				port = DEF_PORT;
			}
			break;
		// MODO CLIENTE
		case 5: //Se especifica el host
			if(argv[1] != "-h"){
				usage();
			}else if( (argv[3] == "-m") && (argv[4] == "cu") ){
				// Modo UDP
				host = argv[2];
				port = DEF_PORT;
			}else if( (argv[3] == "-m") && (argv[4] == "ct") ){
				// Modo TCP
				host = argv[2];
				port = DEF_PORT;
			}else if(argv[3] == "-p"){
				// Modo UDP (default)
				host = argv[2];
				port = argv[4];
			}else{
				usage();
			}
			break;
		case 7:
			if( (argv[1] != "-h") || (argv[3] != "-p") || (argv[5] != "-m") ){
				usage();
			}else{
				host = argv[2];
				port = argv[4];
				if(argv[6] == "ct"){
					// Modo TCP
				}else if(argv[6] == "cu"){
					// Modo UDP
				}
			}
		default:
			usage();
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
