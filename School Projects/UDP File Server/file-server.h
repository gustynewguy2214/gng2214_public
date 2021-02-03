/* This page contains a client program that can request a file from the server program
 * on the next page. The server responds by sending the whole file.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>

#define SERVER_PORT 9136		/* arbitrary, but client & server must agree */ //Chosen based on Dr. Yuksel's suggestion to use the last digits of a special number.
#define BUF_SIZE 4096		/* block transfer size */

void fatal(char *string)
{
  printf("%s\n", string);
  exit(1);
}

char* geterrno(){
	return strerror(errno);
}

//Including math.h doesn't resolve linking error.
//Eustis also had this problem last year if I remember correctly.
long myround(double x){
    if (x < 0.0) return (long)(x - 0.5);
    else return (long)(x + 0.5);
}
