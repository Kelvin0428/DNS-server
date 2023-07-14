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
#define CACHE
#define ANSWER_NUM 28
#define REQUEST 0
#define ANSWER 1
#define COMMA 46
//print the buffer for debug purposes
void printBuffer(unsigned char *buff, uint8_t messagesize){
  printf("size %u\n",messagesize);
  for(int i=0;i<messagesize;i++){
    printf("%x, ",buff[i]);
  }
}

// find the domain name length of a query or response
int findQ_DomainLen(unsigned char* buff, int *act_len, int size, int full){
  int len = 0;
  int index;
  //if this function is called for extracting purposes
  if(full == 0){
    index = 12;
  }
  //if this function is called for caching purposes
  else{
    index = 14;
  }
  unsigned int follow = buff[index];
  while (follow != 0){
    *act_len += follow;
    len += follow;
    index += follow + 1;
    follow = buff[index];
    len += 1;
  }
  return len;
}

//find the domain name of a query or response
void findQ_DomainName(unsigned char*buff, unsigned char *domainName, int full){
  int j = 0;
  int index;
  if(full == 0){
   index = 12;
  }else{
   index = 14;
  }
  unsigned int follow = buff[index];
  while (follow != 0){
    for(int i=index+1;i<=index+follow;i++){
      domainName[j] = buff[i];
      j+=1;
    }
    index += follow + 1;
    follow = buff[index];
    if(follow != 0){
      domainName[j] = COMMA;
      j += 1;
    }
  }
  domainName[j] = '\0';
}
//find the number of answers in a response
int findNum_answer(unsigned char* buff, int size){
  uint8_t num = buff[6] + buff[7];
  return num;
}
//check if a message is AAAA
int messageisAAAA(unsigned char *buff, int size, int begin){
  char myArray[] = { 0x00, 0x1c };
  char myArray2[] = {buff[begin+1],buff[begin+2]};
  return memcmp(myArray,myArray2,sizeof(myArray));
}

//log the event where the query is not AAAA
void log_unimplemented(){
  FILE *f;
  f = fopen("dns_svr.log", "a+");// a+ (create + append) option will allow appending which is useful in a log file
  if (f == NULL) { printf("ERROR");}
  char result[80];
  time_t t;
  t = time(NULL);
  strftime(result,sizeof(result),"%FT%T%z",localtime(&t));
  fprintf(f, "%s unimplemented request\n",result);
  fclose(f);
}

//log the event of something being requested
void log_requested(unsigned char *domainName){
  FILE *f;
  f = fopen("dns_svr.log", "a+"); // a+ (create + append) option will allow appending which is useful in a log file
  if (f == NULL) { printf("ERROR");}
  char result[80];
  time_t t;
  t = time(NULL);
  strftime(result,sizeof(result),"%FT%T%z",localtime(&t));
  fprintf(f, "%s requested %s\n",result, domainName);
  fclose(f);
}

//log the event of something being responded
void log_res(unsigned char *domainName, unsigned char *address){
  FILE *f;
  f = fopen("dns_svr.log", "a+"); // a+ (create + append) option will allow appending which is useful in a log file

  if (f == NULL) { printf("ERROR");}
  char dst[INET6_ADDRSTRLEN+1];
  inet_ntop(AF_INET6,address, dst,INET6_ADDRSTRLEN+1);
  char result[80];
  time_t t;
  t = time(NULL);
  strftime(result,sizeof(result),"%FT%T%z",localtime(&t));
  fprintf(f, "%s %s is at %s\n",result,domainName,dst);
  fclose(f);
}

//check if a message is AAAA and log
bool isAAAA(unsigned char*domainName,unsigned char *buff, int size, int len,int type){
  if(messageisAAAA(buff,size,len) != 0){
    if(type == 0){
      log_requested(domainName);
      log_unimplemented();
    }
    return false;
  }else{
    return true;
  }
}

//find the class of a message
void findQ_class(unsigned char* buff, unsigned char*class,int len){
  class[0] = buff[len+1];
  class[1] = buff[len+2];
}

//find the time to live of an asnwer
void find_ttl(unsigned char* buff, unsigned char*ttl,int begin){
  for(int i=0;i<4;i++){
    ttl[i]=buff[begin + i + 1];
  }
}

//find the data lengt of a request or answer
void find_datalen(unsigned char*buff, unsigned char*datalen, int begin){
  datalen[0] = buff[begin + 1];
  datalen[1] = buff[begin + 2];
}

//find the ip address of an answer
void find_address(unsigned char*buff, unsigned char*address, int begin){
  for(int i=0;i<16;i++){
    address[i]=buff[begin + i + 1];
  }
}

//extracting data from a message and log appropriately
bool extractdata(unsigned char* buff, int size,int type){
  int num_answer = 0;
  bool QisAAAA = false;
  int domainName_len = 0;
  int act_len = 0;
  domainName_len = findQ_DomainLen(buff, &act_len, size,0);
  unsigned char domainName[domainName_len];
  findQ_DomainName(buff,domainName,0);
  QisAAAA = isAAAA(domainName,buff,size,domainName_len+12,REQUEST);
  unsigned char Qclass[2] = "";
  findQ_class(buff,Qclass,domainName_len+12+2);
  if(type == 1){
     num_answer = findNum_answer(buff,size);
     for(int k = 0;k<num_answer;k++){
       int firstanswer_begin = domainName_len + 12 + 4 +k*ANSWER_NUM;
       bool AisAAAA= isAAAA(domainName,buff,size,firstanswer_begin+2,ANSWER);
       unsigned char Aclass[2] = "";
       findQ_class(buff,Aclass,firstanswer_begin+ 2 + 2);
       unsigned char ttl[4] = "";
       find_ttl(buff,ttl,firstanswer_begin +2 +2+ 2);
       unsigned char datalen[2] = "";
       find_datalen(buff,datalen,firstanswer_begin +2 +2+ 2 + 4);
       unsigned char address[16] = "";
       find_address(buff,address,firstanswer_begin+2+2+2+4+2);
       if(k==0 && AisAAAA){
         log_res(domainName, address);
       }

     }
  }else{
    if(QisAAAA){
      log_requested(domainName);
    }
  }
  return QisAAAA;
  }
