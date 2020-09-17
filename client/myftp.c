// Programming Assigment 2
// Brendan Sailer (bsailer1)
// Brennen Hogan  (bhogan1)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#define MAX_LINE 4096

void client(char*, int);

int main(int argc, char * argv[]){
	if(argc == 3) {
		char *host = argv[1];
		int port = atoi(argv[2]);
		printf("Connecting to %s on port %d", host, port);
		client(host, port);
	}
	else {
		fprintf(stderr, "usage: myftp (host) (port)\n");
		exit(1);
	}
}

void client(char* host, int port){
	FILE *fp;
	struct hostent *hp;
	struct sockaddr_in sin;
	char buf[MAX_LINE];
	int s;
	int len;

	/* translate host name into peer's IP address */
	hp = gethostbyname(host);
	if (!hp) {
		fprintf(stderr, "myftp: unknown host: %s\n", host);
		exit(1);
	}
	
	/* build address data structure */
	bzero((char *)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
	sin.sin_port = htons(port);

	/* active open */
	if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("myftp: socket");
		exit(1);
	}

	printf("Welcome to your first TCP client! To quit, type \'Exit\'\n");
	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0){
		perror("myftp: connect");
		close(s);
		exit(1);
	}

	printf("Connection established.\n");
	/* main loop: get and send lines of text */
	while (fgets(buf, sizeof(buf), stdin)){
		buf[MAX_LINE-1] = '\0';
		if (!strncmp(buf, "Exit",4)){
			printf("Good Bye!\n");
			break;
		}
		printf("SENDING: %s", buf);
		len = strlen(buf) + 1;
		if(send(s, buf, len, 0)==-1){
			perror("client send error!");
			exit(1);
		}

		int len;
		char reply[BUFSIZ];
		if((len=recv(s, reply, sizeof(reply), 0))==-1){
			perror("myftp: error receiving reply");
			exit(1);
		}

		printf("Reply: %s\n", reply);
		bzero((char *)&buf, sizeof(buf));
		bzero((char *)&reply, sizeof(reply));
	}

	close(s); 
}
