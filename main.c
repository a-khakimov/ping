#include <stdlib.h>
#include <stdio.h>
#include "ping.h"

/* Usage
 * */

int main(int argc, char** argv)
{
	if(argc < 3) {
		printf("Usage:\n\t%s <host> <timeout> \n", argv[0]);
		return EXIT_FAILURE;
	}

	const unsigned long timeout = atoi(argv[2]);
	const char* const host = argv[1];
	unsigned long reply_time = 0;

	const int result = ping(host, timeout, &reply_time);

	if (result == -1) {
		printf("Host is not available.\n");
		return EXIT_FAILURE;
	} else if (result == 1) {
		printf("Timeout.\n");
	} else {
		printf("Ping host=%s timeout=%lu ", host, timeout);
		printf("time=%lu ms \n", reply_time);
	}

	return EXIT_SUCCESS;
}
