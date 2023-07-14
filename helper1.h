#include <stdbool.h>
void printBuffer(unsigned char *buff, uint8_t messagesize);
int findQ_DomainLen(unsigned char* buff, int *act_len, int size,int full);
void findQ_DomainName(unsigned char*buff, unsigned char *domainName, int full);
int findNum_answer(unsigned char* buff, int size);
int messageisAAAA(unsigned char *buff, int size, int begin);
void log_unimplemented();
void log_requested(unsigned char *domainName);
void log_res(unsigned char *domainName, unsigned char *address);
bool isAAAA(unsigned char* domainName, unsigned char *buff, int size, int len,int type);
void findQ_class(unsigned char* buff, unsigned char*class,int len);
void find_ttl(unsigned char* buff, unsigned char*ttl,int begin);
void find_datalen(unsigned char*buff, unsigned char*datalen, int begin);
void find_address(unsigned char*buff, unsigned char*address, int begin);
bool extractdata(unsigned char* buff, int size, int type);
