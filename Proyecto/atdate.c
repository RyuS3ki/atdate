#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#define UDP_CLIENT 0
#define TCP_CLIENT 1
#define SERVER_MODE 2
#define BACKLOG 10	 // Max pending connections in queue
#define BUFSIZE 32  // Buffer size (TIME payload size)
#define STIME_PORT 37 // Default port for TIME protocol servers
#define SERV_PORT 6649  // Default port for atdate server
/* Time is calculated in seconds, so the count is:
 * 70 years/4 = 17,5 -> 17 leap years in 70 years
 * 70-17 = ((53*365)+(17*366))*24*60*60 seconds
 * Total adjust time = 2208988800
 * We need to substract this quantity to the host Time
 * so that it is adjusted to the RFC
 */
#define LINUX_TIMEBASE 2208988800 // January 1st, 00.00 am 1970

/* Usage function */
void usage(){
	fprintf(stderr, "usage: atdate [-h serverhost] [-p port] [-m cu|ct|s] [-d]\n");
	exit(1);
}

/* Signal handler for Ctrl-C */
void ctrlc_handler(int sig){
  printf("Ctrl-C captured, exiting...\n");
  printf("See you soon!\n");
  exit(0);
}

/* Signal handler for ended child processes */
void sigchld_handler(int s){
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

/* TCP Server function */
int tcp_server(int debug){
  signal(SIGCHLD, sigchld_handler);

  int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	int port = SERV_PORT;
  int clientlen; // byte size of client's address
  struct sockaddr_in serveraddr; // server's addr
  struct sockaddr_in clientaddr; // client addr
  struct hostent *hostp; // client host info
  char *hostaddrp; // dotted decimal host addr string
  int optval; // flag value for setsockopt

  /* socket: create the parent socket */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("ERROR opening socket");
		exit(0);
	}

  /* setsockopt: lets s rerun the server immediately after we kill it.
   * Eliminates "ERROR on binding: Address already in * use" error.
   */
  optval = 1;
  setsockopt(new_fd, SOL_SOCKET, SO_REUSEADDR,(const void *)&optval , sizeof(int));

  /* build the server's Internet address */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET; // IPv4
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)port);

  /* bind: associate the parent socket with a port */
  if (bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
    perror("ERROR on binding");
		exit(0);
	}

  /* listen: make this socket ready to accept connection requests */
  if (listen(sockfd, BACKLOG) < 0) {
		perror("ERROR on listen");
		exit(0);
	}

  /* wait for a connection request */
	while(1) {  // main accept() loop
    clientlen = sizeof(clientaddr);
		new_fd = accept(sockfd, (struct sockaddr *)&clientaddr, &clientlen);
		if (new_fd == -1) {
			perror("ERROR on accept");
			exit(0);
		}

		/* gethostbyaddr: determine who sent the message */
    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL) {
      perror("ERROR on gethostbyaddr");
			exit(0);
		}
    hostaddrp = inet_ntoa(clientaddr.sin_addr); // Net to ASCII (client addr)
    if (hostaddrp == NULL) {
      perror("ERROR on inet_ntoa\n");
			exit(0);
		}
    //printf("server got connection from %s (%s)\n", hostp->h_name, hostaddrp);

    if(!fork()){ // this is the child process: fork() == 0
      uint32_t time_send; // 32 bit long int
      int n;
      time_t own_time;

      do{
        own_time = time(NULL); // Getting the server's time
        uint32_t time_send = htonl(own_time - LINUX_TIMEBASE); // Adjusted
        /* Sending time */
        n = send(new_fd, &time_send, sizeof(uint32_t), 0);
        if(n<0){
          fprintf(stderr, "ERROR sending\n");
        }else{
          printf("Date & time correctly sent\n");
        }
        sleep(1); // Wait 1 second after sending new time
      }while(n>0); // If n<0 -> Client closed connection

      // Closed connection
      close(sockfd); // child doesn't need the listener
			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}
	return 0;
}

/* TCP Client function */
int tcp_client(char *host, int port, int debug){

  int clientfd;
  struct hostent *server;
	struct sockaddr_in serveraddr;
  uint32_t buf; // Variable to store the information received
  int n;

  signal(SIGINT, ctrlc_handler);

  /* socket: IPv4, STREAM + default protocol = TCP */
  clientfd = socket(AF_INET, SOCK_STREAM, 0);
  if (clientfd < 0) {
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
  if (connect(clientfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
    perror("ERROR connecting");
    exit(0);
  }

	/* read answer from the server */
  n = recv(clientfd, &buf, BUFSIZE, 0);
  if (n < 0) {
    perror("ERROR receiving");
    exit(0);
  }else{
    struct tm *final_date; // Struct tm with rcvd time
    char *final_date_s; // Buffer to store formatted string
    time_t t_rcvd = ntohl(buf) + LINUX_TIMEBASE; // Adjust to linux
    final_date = localtime(&t_rcvd);
    strftime(final_date_s, 80, "%c", final_date);

    /* print the server's reply */
    printf("%s\n", final_date_s);
  }

  /* Close socket: ended connection */
  close(clientfd);
	return(0);

}

/* UDP Client function */
int udp_client(char *host, int port, int debug){

  int clientfd;
  struct hostent *server;
	struct sockaddr_in serveraddr;
  uint32_t buf; // Variable to store the information received
  int n;

  signal(SIGINT, ctrlc_handler);

  /* socket: IPv4, DGRAM + default protocol = UDP */
  if(debug) printf("Creating socket\n");
  clientfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (clientfd < 0) {
    perror("ERROR opening socket");
    exit(0);
  }

  /* gethostbyname: get the server's DNS entry */
  if(debug) printf("Getting server address\n");
  server = gethostbyname(host);
  if (server == NULL) {
    fprintf(stderr,"ERROR, no such host: %s\n", host);
    exit(0);
  }

  if(debug) printf("Storing server address\n");
  /* build the server's Internet address */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr, server->h_length);
  serveraddr.sin_port = htons(port);

  /* If the method "connect" is used at this point, we could use send and recv,
   * instead of sendto and recvfrom
   */

  if(debug) printf("Sending empty packet\n");
  /* Send empty message to server */
  n = sendto(clientfd, NULL, 0, 0, (struct sockaddr *) &serveraddr, (socklen_t) sizeof(serveraddr));
  if (n < 0) {
    perror("ERROR sending empty packet");
    exit(0);
  }
  if(debug) printf("Sent!\n");

  if(debug) printf("Waiting for server's answer\n");
  /* read answer from the server */
  n = recvfrom(clientfd, &buf, sizeof(uint32_t), 0, (struct sockaddr *) &serveraddr, (socklen_t *) sizeof(serveraddr));
  if (n < 0) {
    perror("ERROR reading from socket");
    exit(0);
  }
  if(debug) printf("Data received\n");

  if(n>0){ // Data received from server
    struct tm *final_date; // Struct tm with rcvd time
    char *final_date_s; // Buffer to store formatted string
    time_t t_rcvd = ntohl(buf) + LINUX_TIMEBASE; // Adjust to linux
    final_date = localtime(&t_rcvd);
    strftime(final_date_s, 80, "%c", final_date);

    /* print the server's reply */
    printf("%s\n", final_date_s);
  }

  if(debug) printf("Closing client\n");
  /* Close socket: ended connection */
  close(clientfd);
	return(0);

}

int main(int argc, char* const argv[]) {
  int opt;
  int debug = 0;
  int mode;
  //int mode = UDP_CLIENT; // Default mode
  char* host;
	int port = STIME_PORT;

  /* Parsing command-line arguments
   * Examples used from:
   * https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html#Example-of-Getopt
   */
  while((opt = getopt(argc, argv, "d?h:p:m:")) != -1){
    switch(opt){
      case 'h':
        host = optarg;
        break;

      case 'p':
        port = atoi(optarg); // Convert input(ASCII) to int
        break;

      case 'm':
        if(strcmp(optarg,"cu") == 0){
          if(debug) printf("UDP mode\n");
          //mode = UDP_CLIENT;
        }else if(strcmp(optarg,"ct") == 0){
          if(debug) printf("TCP mode\n");
          mode = TCP_CLIENT;
        }else if(strcmp(optarg,"s") == 0){
          mode = SERVER_MODE;
        }
        break;

      case 'd':
        debug = 1; // Debug set to true
        break;

      case '?':
        usage();
    }
  }

  if(mode =! 2 && host == NULL){
    printf("To execute Client Mode you must specify at least the option '-h'\n");
    usage();
  }else{
    printf("Host server is: %s\n", host);
    printf("Port is: %s\n", port);
  }

  if (mode == UDP_CLIENT) {
    if(debug) printf("Executing UDP Client\n");
    udp_client(host, port, debug);
  }else if(mode == TCP_CLIENT){
    if(debug) printf("Executing TCP Client\n");
    tcp_client(host, port, debug);
  }else{
    if(debug) printf("Executing Server\n");
    tcp_server(debug);
  }

  return 0;
}
