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
  printf("\nCtrl-C captured, exiting...\n");
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

  printf("TIME server running in port %d\n", port);

  /* socket: create the parent socket */
  if(debug) printf("Creating socket for petitions\n");
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("ERROR opening socket");
		exit(0);
	}

  /* setsockopt: lets s rerun the server immediately after we kill it.
   * Eliminates "ERROR on binding: Address already in * use" error.
   */
  if(debug) printf("Options: accept connections in a different socket\n");
  optval = 1;
  setsockopt(new_fd, SOL_SOCKET, SO_REUSEADDR,(const void *)&optval , sizeof(int));

  /* build the server's Internet address */
  if(debug) printf("Creating address\n");
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET; // IPv4
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)port);

  /* bind: associate the parent socket with a port */
  if(debug) printf("Bind: Specify local address (with port)\n");
  if (bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
    perror("ERROR on binding");
		exit(0);
	}

  /* listen: make this socket ready to accept connection requests */
  if(debug) printf("Listen: Ready to get connections\n");
  if (listen(sockfd, BACKLOG) < 0) {
		perror("ERROR on listen");
		exit(0);
	}

  /* wait for a connection request */
	while(1) {  // main accept() loop
    clientlen = sizeof(clientaddr);
    if(debug) printf("Accepting new connection\n");
		new_fd = accept(sockfd, (struct sockaddr *)&clientaddr, &clientlen);
		if (new_fd == -1) {
			perror("ERROR on accept");
			exit(0);
		}
    if(debug) printf("New connection accepted\n");

		/* gethostbyaddr: determine who sent the message */
    if(debug) printf("getting client's address\n");
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
      if(debug) printf("New child process generated\n");
      uint32_t time_send; // 32 bit long int
      int n;
      time_t own_time;

      do{
        if(debug) printf("Getting system time\n");
        own_time = time(NULL); // Getting the server's time
        if(debug) printf("Adjusting time to NTP timebase\n");
        uint32_t time_send = htonl(own_time + LINUX_TIMEBASE); // Adjusted
        /* Sending time */
        if(debug) printf("Sending...\n");
        n = send(new_fd, &time_send, sizeof(uint32_t), 0);
        if(n<0){
          fprintf(stderr, "ERROR sending\n");
        }else{
          printf("Date & time correctly sent\n");
        }
        sleep(1); // Wait 1 second after sending new time
      }while(n>0); // If n<0 -> Client closed connection

      // Closed connection
      if(debug) printf("Closing connection with client\n");
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

  printf("Starting TCP\n");
  int clientfd;
  struct hostent *server;
	struct sockaddr_in serveraddr;
  uint32_t buf; // Variable to store the information received
  int n;

  signal(SIGINT, ctrlc_handler);

  /* socket: IPv4, STREAM + default protocol = TCP */
  if(debug) printf("Creating socket\n");
  clientfd = socket(AF_INET, SOCK_STREAM, 0);
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

  /* build the server's Internet address */
  if(debug) printf("Storing server address\n");
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr, server->h_length);
  serveraddr.sin_port = htons(port);

  /* connect: create a connection with the server */
  if (debug) printf("Creating connection\n");
  if (connect(clientfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
    perror("ERROR connecting");
    exit(0);
  }

  if(debug) printf("Waiting for server's answer\n");
	/* read answer from the server */
  while(1){
    n = recv(clientfd, &buf, BUFSIZE, 0);
    if (n < 0) {
      perror("ERROR receiving");
      exit(0);
    }else{
      if(debug) printf("Data received\n");
      time_t t_rcvd = ntohl(buf) - LINUX_TIMEBASE; // Adjust to linux
      if(debug) printf("Adjusting time to linux timebase\n");
      struct tm *final_date; // Struct tm with rcvd time
      //final_date = localtime(&t_rcvd);
      if(debug) printf("Formatting date\n");
      char final_date_s[80]; // Buffer to store formatted string
      strftime(final_date_s, 80, "%a %b %d %X %Z %Y", localtime(&t_rcvd));

      /* print the server's reply */
      if(debug) printf("Printing date\n");
      printf("%s\n", final_date_s);
    }
  }

  /* Close socket: ended connection */
  if(debug) printf("Closing client\n");
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

  /* If the method "connect" is not used at this point, we'll use sendto and
   * recvfrom, instead of send and recv
   */

  /* connect: create a connection with the server */
  if (debug) printf("Creating connection\n");
  if (connect(clientfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
    perror("ERROR connecting");
    exit(0);
  }

  if(debug) printf("Sending empty packet\n");
  /* Send empty message to server */
  n = send(clientfd, NULL, 0, 0);
  if (n < 0) {
    perror("ERROR sending empty packet");
    exit(0);
  }
  if(debug) printf("Sent!\n");

  if(debug) printf("Waiting for server's answer\n");
  /* read answer from the server */
  n = recv(clientfd, &buf, sizeof(buf), 0);
  if (n < 0) {
    perror("ERROR reading from socket");
    exit(0);
  }else{
    if(debug) printf("Data received\n");
    time_t t_rcvd = ntohl(buf) - LINUX_TIMEBASE; // Adjust to linux
    if(debug) printf("Adjusting time to linux timebase\n");
    struct tm *final_date; // Struct tm with rcvd time
    //final_date = localtime(&t_rcvd);
    if(debug) printf("Formatting date\n");
    char final_date_s[80]; // Buffer to store formatted string
    strftime(final_date_s, 80, "%a %b %d %X %Z %Y", localtime(&t_rcvd));

    /* print the server's reply */
    if(debug) printf("Printing date\n");
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
  char* mode = NULL;
  char* host = NULL;
	int port = 37;
  char *str_opt;

  /* Parsing command-line arguments
   * Examples used from:
   * https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html#Example-of-Getopt
   */
  while((opt = getopt(argc, argv, "dh:p:m:")) != -1){
    switch(opt){
      case 'h':
        host = optarg;
        break;

      case 'p':
        port = atoi(optarg); // Convert input(ASCII) to int
        printf("Port selected is: %d\n", port);
        break;

      case 'm':
        mode = optarg;
        break;

      case 'd':
        debug = 1; // Debug set to true
        break;

      case '?':
        usage();
    }
  }

  if(mode == NULL){ // Default mode
    if(debug) printf("No mode specified. Setting default mode\n");
    mode = "cu";
  }

  if((strcmp(mode,"cu") == 0) && host != NULL){
    if(debug) printf("Executing UDP Client\n");
    udp_client(host, port, debug);
  }else if(host == NULL){
    printf("To execute Client Mode you must specify at least the option '-h'\n");
    usage();
  }

  if((strcmp(mode,"ct") == 0) && host != NULL){
    if(debug) printf("Executing TCP Client\n");
    tcp_client(host, port, debug);
  }else if(host == NULL){
    printf("To execute Client Mode you must specify at least the option '-h'\n");
    usage();
  }

  if(strcmp(mode,"s") == 0){
    if(debug) printf("Executing Server\n");
    tcp_server(debug);
  }

  return 0;
}
