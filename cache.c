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
#include "cache.h"
#include "server.h"
#define CACHE
//creates a new cache element in the cache array
Cache * newEl(){
  Cache *cache = malloc(sizeof(cache));
  time_t default_t;
  time(&default_t);
  cache->foundtime = default_t;
  cache->endtime = default_t;
  cache->response = (unsigned char*)malloc(400* sizeof(unsigned char));
  cache->size = 2;
  cache->ttl = -1;
  return cache;
}
//initialise every cache element in the cache array
void newCache(Cache **cache){
  for(int i=0;i<5;i++){
    cache[i] = newEl();
  }
}
//printing the cache eviction log
void log_evict(unsigned char*domainName,unsigned char*domainNameNew){
  FILE *f;
  f = fopen("dns_svr.log", "a+"); // a+ (create + append) option will allow appending which is useful in a log file
  if (f == NULL) { printf("ERROR");}
  char result[80];
  time_t t;
  t = time(NULL);
  strftime(result,sizeof(result),"%FT%T%z",localtime(&t));
  fprintf(f, "%s replacing %s by %s\n",result, domainName, domainNameNew);
  fclose(f);
}
//adding the input "sended" response into cache of index
void addCache(unsigned char * sended, Cache **cache, int messagesize, int index){
  time_t found_t;
  time(&found_t);
  //if the current cache is not empty
  //evict the current cache entry
  if(cache[index]->ttl != -1){
    int domainNameNew_len = 0;
    int actNew_len = 0;
    domainNameNew_len = findQ_DomainLen(sended, &actNew_len, messagesize,1);
    unsigned char domainNameNew[domainNameNew_len];
    findQ_DomainName(sended,domainNameNew,1);
    int domainName_len = 0;
    int act_len = 0;
    domainName_len = findQ_DomainLen(cache[index]->response, &act_len, messagesize,1);
    unsigned char domainName[domainName_len];
    findQ_DomainName(cache[index]->response,domainName,1);
    log_evict(domainName,domainNameNew);
  }
  //replace the current cache entry with the new response
  cache[index]->foundtime = found_t;
  cache[index]->size = messagesize;
  int domainName_len2 = 0;
  int act_len2 = 0;
  domainName_len2 = findQ_DomainLen(sended, &act_len2, messagesize,1);
  int firstanswer_begin = domainName_len2 + 12 + 4+2;
  unsigned char ttl[4] = "";
  find_ttl(sended,ttl,2 + firstanswer_begin +2 +2);
  cache[index]->ttl = (uint32_t)((ttl[0] << 24) | (ttl[1] << 16) | (ttl[2] << 8) | (ttl[3] << 0));
  cache[index]->endtime = found_t + cache[index]->ttl;
  for(int i=0;i<messagesize;i++){
    cache[index]->response[i] = sended[i];
  }

}
//put the response into a valid space in cache
void put_in_cache(unsigned char *sended, Cache **cache, int messagesize){
  bool new = false;
  uint8_t num_answer = 0;
  num_answer = sended[8]<<8|sended[9];
  //if the response has no answers, dont cache it
  if(num_answer == 0){
    return;
  }
  //if the response isnt added through eviction by expiry, find if there is
  //an index of the cache that doesnt have a response
  if(!check_expiration(sended,cache,messagesize)){
    for(int i=0;i<5;i++){
      if(cache[i]->ttl == -1){
        addCache(sended,cache,messagesize,i);
        new = true;
        break;
      }
    }
    //if there doesnt exist an expiried response and cache is full, evict the first
    //element of response  in the cache
    if(new == false){
      addCache(sended,cache,messagesize,0);
    }
  }
}
//check in the cache there is an expired response, if so, replace with new resposnse
bool check_expiration(unsigned char*sended, Cache **cache,int messagesize){
  time_t check_t;
  time(&check_t);
  for(int i=0;i<5;i++){
    //if the current response exists and the difference in time of current and the time
    //when the response is added to the cache is larger than the time to live
    // replace the response
    if(cache[i]->ttl != -1 && difftime(check_t,cache[i]->foundtime) > cache[i]->ttl){
      addCache(sended,cache,messagesize,i);
      return true;
    }
  }
  return false;
}
//print cache and its ttl out for debug purposes
void printCache(Cache **cache){
  for(int i=0;i<5;i++){
    printf("Cache %d, %d",i,cache[i]->ttl);
  }
}
//log the action of finding the response in cache
void log_inCache(unsigned char*domainName,time_t endt){
  FILE *f;
  f = fopen("dns_svr.log", "a+"); // a+ (create + append) option will allow appending which is useful in a log file
  if (f == NULL) { printf("ERROR");}
  char result[80];
  time_t t;
  t = time(NULL);
  strftime(result,sizeof(result),"%FT%T%z",localtime(&t));
  char endresult[80];
  strftime(endresult,sizeof(endresult),"%FT%T%z",localtime(&endt));
  fprintf(f, "%s %s expires at %s\n",result, domainName, endresult);
  fclose(f);
}
//look in to the cache and find if there is a response for the request
unsigned char* lookin_cache(unsigned char*send, Cache **cache, int messagesize,bool *returned){
  int domainNameNew_len = 0;
  int actNew_len = 0;
  domainNameNew_len = findQ_DomainLen(send, &actNew_len, messagesize,1);
  unsigned char domainNameNew[domainNameNew_len];
  findQ_DomainName(send,domainNameNew,1);
  //for each of the response in cache
  for(int i=0;i<5;i++){
    time_t check_t;
    time(&check_t);
    //if the response exists and is not expired
    if(cache[i]->ttl != -1 && difftime(check_t,cache[i]->foundtime) < cache[i]->ttl){
      int domainName_len = 0;
      int act_len = 0;
      domainName_len = findQ_DomainLen(cache[i]->response, &act_len, messagesize,1);
      unsigned char domainName[domainName_len];
      findQ_DomainName(cache[i]->response,domainName,1);
      //if the domain name of response is the same with the domain name of the request
      if(memcmp(domainNameNew, domainName, domainName_len) == 0){
        double diff = difftime(check_t, cache[i]->foundtime);
        int ttl_after = cache[i]->ttl - diff;
        //getting the new time to live
        unsigned char bytes[4];
        bytes[0] = (ttl_after >> 24) & 0xFF;
        bytes[1] = (ttl_after >> 16) & 0xFF;
        bytes[2] = (ttl_after >> 8) & 0xFF;
        bytes[3] = ttl_after & 0xFF;
        int begin = domainName_len + 14 +4 + 6+1;
        //changing the transaction id and the ttl of the response
        cache[i]->response[2] = send[2];
        cache[i]->response[3] = send[3];
        cache[i]->response[begin] = bytes[0];
        cache[i]->response[begin+1] = bytes[1];
        cache[i]->response[begin+2] = bytes[2];
        cache[i]->response[begin+3] = bytes[3];
        *returned = true;
        log_inCache(domainName,cache[i]->endtime);
        return cache[i]->response;
      }
    }
  }
  //didn't find a valid response for the request
  *returned = false;
  return NULL;


}
