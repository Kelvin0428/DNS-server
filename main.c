#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include "helper1.h"
#include "server.h"
#include "sys/stat.h"
#define CACHE
//thought process, make server, get query,log, be client ,send query to input ip and port, get response, log, send response
//main function that runs the server
int main(int argc, char* argv[]) {

	/* CL args */
  if (argc < 3) {
		fprintf(stderr, "usage: %s ip port\n", argv[0]);
		return 0;
	}
	run_server(argv[1],argv[2]);

	return 0;
}
