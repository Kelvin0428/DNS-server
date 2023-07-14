

void run_server(char * ip, char * port);
int setup_client_socket(const int port, const char* server_name,
						struct sockaddr_in* serv_addr);
int create_server_socket(const int port);
void respond_error(int i,unsigned char *buf);
int read_rest(int i, uint8_t messagesize, unsigned char * buf,unsigned char*send);
