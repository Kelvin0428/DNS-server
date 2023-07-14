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
#include "helper1.h"
  //------------------------------------------

int main(int argc, char* argv[]) {
  FILE *f;
  f = fopen("dns_svr.log", "a+");
  int permission = chmod("dns_svr.log", 0644);
  if(permission == -1){
    printf("ERROR in changing permission");
  }
//------Reading in packet into buffer---------------
  unsigned char buf[2];
  uint8_t messagesize = 0;
  int v = read(STDIN_FILENO, buf, 2);
  if (v < 0) {
    perror("read");
    exit(EXIT_FAILURE);
  }
  messagesize = buf[0]<<8|buf[1];
  unsigned char buff[messagesize];
  read(STDIN_FILENO, buff, messagesize);
  printBuffer(buff,messagesize);
  printf(" message %d\n",messagesize);
//-----------------------------------------------
 char *array = "response";
 if(strcmp(array,argv[1]) == 0){
     extractdata(buff,messagesize,1);
 }else{
     extractdata(buff,messagesize,0);
 }

//---------printing buffer out to test--------
//  printBuffer(buff,messagesize);
//  printf("testing %s\n",argv[1]);
//-------------------------------------------

//--------testing ntohl-------------------
/*  uint32_t result = ntohl(*(uint32_t*)&buff[4]);
  printf("The result: %u\n", result);*/
    return 0;
}


//need to read in bytes based on format for request, and for response

//if request, we assume only 1 question, check bits for A or AAAA, get the name

//if response, we do the same as if it is request,also check number of answers, 1 answer is 28 sets of bits, check type of answer, only log first answers
// if it is AAAA.
//get the address, get the name
