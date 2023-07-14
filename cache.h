typedef struct cache Cache;
struct cache{
  time_t foundtime;
  time_t endtime;
  unsigned char *response;
  int size;
  uint32_t ttl;
};
//creates a new cache element in the cache array
Cache * newEl();

//initialise every cache element in the cache array
void newCache(Cache **cache);

//printing the cache eviction log
void log_evict(unsigned char*domainName,unsigned char*domainNameNew);

//adding the input "sended" response into cache of index
void addCache(unsigned char * sended, Cache **cache, int messagesize, int index);

//put the response into a valid space in cache
void put_in_cache(unsigned char *sended, Cache **cache, int messagesize);

//check in the cache there is an expired response, if so, replace with new resposnse
bool check_expiration(unsigned char*sended, Cache **cache,int messagesize);

//print cache and its ttl out for debug purposes
void printCache(Cache **cache);

//log the action of finding the response in cache
void log_inCache(unsigned char*domainName,time_t endt);

//look in to the cache and find if there is a response for the request
unsigned char* lookin_cache(unsigned char*send, Cache **cache, int messagesize,bool *returned);
