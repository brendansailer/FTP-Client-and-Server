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

void client(char*, int);
void send_fn(int, char []);
void recv_fn(int, char []);

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
	struct hostent *hp;
	struct sockaddr_in sin;
	char buf[BUFSIZ];
	char reply[BUFSIZ];
	int s;

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
		buf[BUFSIZ-1] = '\0';
		if (!strncmp(buf, "Quit", 4)){
			printf("Good Bye!\n");
			break;
		}

		char *command = strtok(buf, " ");
		char *arg1 = strtok(NULL, " ");

		if(strcmp(buf, "MKDIR\n") == 0){
			send_fn(s, buf);
			recv_fn(s, reply);
			if(strcmp(reply, "-2") == 0){
				printf("The directory already exists on server\n");
			} else if(strcmp(reply, "-1") == 0){
				printf("Error in making directory\n");
			} else{
				printf("The directory was successfully made\n");
			}
		} else if(strcmp(buf, "RMDIR\n") == 0){
		
		} else {
			printf("SENDING: %s", buf);
			send_fn(s, buf);

			recv_fn(s, reply);
			printf("Reply: %s\n", reply);
		}
		
		bzero((char *)&buf, sizeof(buf));
		bzero((char *)&reply, sizeof(reply));
	}

	close(s); 
}

void send_fn(int socket, char buf[]){
	if(send(socket, buf, strlen(buf)+1, 0)==-1){
		printf("Client send error");
	}
}

void recv_fn(int socket, char reply[]){
	int length;
	if((length=recv(socket, reply, sizeof(reply), 0))==-1){
		perror("myftp: error receiving reply");
	}
	printf(reply);
}
