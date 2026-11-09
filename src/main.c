#include "server.h"
#include <string.h>

int main(int argc, char** argv)
{
	int port = 8080;
	for(int i = 0;i < argc; i++){
		if(strcmp("-p", argv[i]) == 0 && i+1 < argc){
			port = atoi(argv[i+1]);
			break;
		}
	}

	init_server();

	run_server();

	return 0;
}
