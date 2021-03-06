/* 
 * udpserver.c - A simple UDP echo server 
 * usage: udpserver <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/time.h> 
#include <fcntl.h>
#include <sys/poll.h>
#include <time.h>

#define BUFSIZE 1024

void gettimeofdayoriginal(struct timeval *tv, struct timezone *tz) {
#ifdef __x86_64
  syscall(314, tv, tz);
  return;
#endif
  syscall(351, tv, tz);
  return;
}

void print_time(){

  struct timeval later;
  struct timeval later1;
  struct tm localtm;
  struct tm origtm;

  gettimeofday(&later, NULL);
  gettimeofdayoriginal(&later1, NULL);
  localtime_r(&(later.tv_sec), &localtm);
  localtime_r(&(later1.tv_sec),&origtm);
  //printf("%d %d virtual time: %ld:%ld physical time: %ld:%ld localtime: %d:%02d:%02d %ld\n",x,getpid(),later.tv_sec-now.tv_sec,later.tv_usec-now.tv_usec,later1.tv_sec-now1.tv_sec,later1.tv_usec-now1.tv_usec,localtm.tm_hour, localtm.tm_min, localtm.tm_sec, later.tv_usec);

  printf("curr : localtime: %d:%02d:%02d %ld, orig_time : %d:%02d:%02d %ld\n", localtm.tm_hour, localtm.tm_min, localtm.tm_sec, later.tv_usec, origtm.tm_hour, origtm.tm_min, origtm.tm_sec, later1.tv_usec);
  fflush(stdout);
}


/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char **argv) {
  int sockfd; /* socket */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buf */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */
  int numExpectedPings;
  /* 
   * check command line arguments 
   */
  if (argc != 3) {
    fprintf(stderr, "usage: %s <port> <numPings>\n", argv[0]);
    exit(1);
  }
  
  numExpectedPings = atoi(argv[2]);
  
  portno = atoi(argv[1]);

  /* 
   * socket: create the parent socket 
   */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

   char * s = inet_ntoa(serveraddr.sin_addr);
   printf("My IP address : %s\n",s);
 
  /* 
   * bind: associate the parent socket with a port 
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) 
    error("ERROR on binding");

  /* 
   * main loop: wait for a datagram, then echo it
   */
  int numReceived = 0;
  clientlen = sizeof(clientaddr);
  while (1) {

    /*
     * recvfrom: receive a UDP datagram from a client
     */
    bzero(buf, BUFSIZE);
    n = recvfrom(sockfd, buf, BUFSIZE, 0,
		 (struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0)
      error("ERROR in recvfrom");
	   numReceived++;
	

    /* 
     * gethostbyaddr: determine who sent the datagram
     */
      //hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
	    //		  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
      //if (hostp == NULL)
      //  error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
     //printf("server received datagram from %s (%s)\n", 
	   //hostp->h_name, hostaddrp);

    printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);
    printf("Recv Time : \n");
    print_time();
     
    /* 
     * sendto: echo the input back to the client 
     */
    n = sendto(sockfd, buf, strlen(buf), 0, 
	       (struct sockaddr *) &clientaddr, clientlen);
    if (n < 0) 
      error("ERROR in sendto");
      
    if (numExpectedPings == numReceived)
		break;  
  }
}
