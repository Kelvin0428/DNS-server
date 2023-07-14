#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <time.h>
#include "server.h"
#include "helper1.h"
#include "cache.h"
#define CACHE
#define PORT 8053



//responding a response wirht Rcode 4
void respond_error(int i,unsigned char *buf){
  unsigned char errorsend[14] = {0x00, 0x0c,buf[0],buf[1],0x81,0x84,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
  write(i,errorsend,14);
}

//read rest of the message apart from the first two byte
int read_rest(int i, uint8_t messagesize, unsigned char * buf,unsigned char*send){
  int all = messagesize;
  int cur = 0;
  int index = 0;
  unsigned char newly[messagesize];
  int n;
  send[0] = buf[0];
  send[1] = buf[1];
  while(all != cur){
    n = read(i,newly,messagesize);
    for(int k=0;k<n;k++){
      buf[index] = newly[k];
      index += 1;
    }
    cur += n;
  }
  for(int z=0;z<messagesize;z++){
    send[z+2] = buf[z];
  }
  return n;
}
//setup the client socket
/*The code from this function was inspired by the code in Practical 9 & 10
and consists partially the code from Practical 9&10*/
int setup_client_socket(const int port, const char* server_name,
						struct sockaddr_in* serv_addr) {
	int sockfd;
	struct hostent* server;

	server = gethostbyname(server_name);
	if (!server) {
		fprintf(stderr, "ERROR, no such host\n");
		exit(EXIT_FAILURE);
	}
	bzero((char*)serv_addr, sizeof(serv_addr));
	serv_addr->sin_family = AF_INET;
	bcopy(server->h_addr_list[0], (char*)&serv_addr->sin_addr.s_addr,
		  server->h_length);
	serv_addr->sin_port = htons(port);

	/* Create socket */
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	return sockfd;
}

/*The code from this function was inspired by the code in Practical 9 & 10
and consists partially the code from Practical 9&10*/
void run_server(char*ip,char*porto) {
	int sockfd,client_sockfd;
  unsigned char *buf = (unsigned char *)malloc(2*sizeof(unsigned char));
  unsigned char *buf2 = (unsigned char *)malloc(2*sizeof(unsigned char));
  struct sockaddr_in serv_addr;
  char* server;
  int port;
  port = atoi(porto);
  server = ip;
	sockfd = create_server_socket(PORT);
  Cache *cache[5];
  newCache(cache);
	/* Listen on socket, define max. number of queued requests */
	if (listen(sockfd, 5) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}
  // initialise an active file descriptors set
	fd_set masterfds;
	FD_ZERO(&masterfds);
	FD_SET(sockfd, &masterfds);
	// record the maximum socket number
	int maxfd = sockfd;
	while (1) {
		// monitor file descriptors
		fd_set readfds = masterfds;
		if (select(FD_SETSIZE, &readfds, NULL, NULL, NULL) < 0) {
			perror("select");
			exit(EXIT_FAILURE);
		}
		// loop all possible descriptor
		for (int i = 0; i <= maxfd; ++i){
			// determine if the current file descriptor is active
			if (FD_ISSET(i, &readfds)) {
				// create new socket if there is new incoming connection request
				if (i == sockfd) {
					struct sockaddr_in cliaddr;
					socklen_t clilen = sizeof(cliaddr);
					int newsockfd =	accept(sockfd, (struct sockaddr*)&cliaddr, &clilen);
					if (newsockfd < 0)
						perror("accept");
					else {
						// add the socket to the set
						FD_SET(newsockfd, &masterfds);
						// update the maximum tracker
						if (newsockfd > maxfd)
							maxfd = newsockfd;
					}
				}
				// Code implemented by myself
				else {
          uint8_t messagesize = 0;
          int n = read(i, buf, 2);
          messagesize = buf[0]<<8|buf[1];
          buf = realloc(buf, messagesize);
          unsigned char send[messagesize+2];
          n = read_rest(i,messagesize,buf,send);
					if (n < 0) {
						perror("read");
					}else{
              if(extractdata(buf,messagesize,0)){
                bool returned = false;
                unsigned char *cache_res = (unsigned char*)malloc(messagesize+2+28 * sizeof(unsigned char));
                cache_res = lookin_cache(send,cache,messagesize+2,&returned);

                //look in cache, check if there is one that fits
                /* Make connection */

                if(returned == true){
                  uint8_t newsize = cache_res[0]<<8|cache_res[1];
                  unsigned char *extract = (unsigned char *)malloc(newsize * sizeof(unsigned char));
                  for(int i=0;i<newsize;i++){
                    extract[i] = cache_res[i+2];
                  }
                  extractdata(extract,newsize,1);
                  free(extract);
                  write(i,cache_res,newsize+2);
                }else{
                  client_sockfd = setup_client_socket(port, server, &serv_addr);
                  if (connect(client_sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) <
                    0) {
                      perror("connect");
                      exit(EXIT_FAILURE);
                    }
                    n = write(client_sockfd, send, messagesize+2);
                    if (n < 0) {
                      perror("write");
                      exit(EXIT_FAILURE);
                    }
                    messagesize = 0;
                    n = read(client_sockfd, buf2, 2);
                    messagesize = buf2[0]<<8|buf2[1];
                    unsigned char sended[messagesize+2];
                    buf2 = realloc(buf2, messagesize);
                    n = read_rest(client_sockfd,messagesize,buf2,sended);
                    if (n < 0) {
                      perror("read");
                      exit(EXIT_FAILURE);
                    }else{
                      put_in_cache(sended,cache,messagesize+2);
                      extractdata(buf2,messagesize,1);
                      write(i,sended,messagesize+2);
                      close(client_sockfd);
                  }
                }
              }else{
                respond_error(i,buf);
              }
            }
						close(i);
						FD_CLR(i, &masterfds);
					}
				}
			}
	}
	close(sockfd);
}

/* Create and return a socket bound to the given port */
/*The code from this function consists the code from Practical 9&10*/
int create_server_socket(const int port) {
	int sockfd;
	struct sockaddr_in serv_addr;

	/* Create socket */
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	/* Create listen address for given port number (in network byte order)
	for all IP addresses of this machine */
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);

	/* Reuse port if possible */
	int re = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &re, sizeof(int)) < 0) {
		perror("Could not reopen socket");
		exit(EXIT_FAILURE);
	}

	/* Bind address to socket */
	if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	return sockfd;
}
