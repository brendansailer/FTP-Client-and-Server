// Programming Assignment 2
// Brendan Sailer (bsailer1)
// Brennen Hogan  (bhogan1)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <dirent.h>
#define MAX_PENDING 5
#define MAX_LINE 4096

void server(int);
void complete_request(int, char []);
void ls(char []);
void mkdir(char *, char []);
void send_fn(int, char [], int);

int main(int argc, char * argv[]) {
		if(argc == 2){
			int port = atoi(argv[1]);
			printf("Waiting for connections on port %d\n", port);
			server(port);
		} else {
			perror("useage: myftpd (port)");
			exit(1);
		}
}

void server(int port){
	struct sockaddr_in sin, client_addr;
	char buf[MAX_LINE];
	int len, addr_len;
	int s, new_s;

	/* build address data structure */
	bzero((char *)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);

	/* setup passive open */
	if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("simplex-talk: socket");
		exit(1);
	}

	// set socket option
	// TODO - the code he gave us was wrong
	int opt = 5000;
	if((setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)& opt, sizeof(int)))<0){
		perror ("simplex-talk:setscokt");
		exit(1);
	}

	if((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0) {
		perror("simplex-talk: bind"); exit(1);
	}
	if ((listen(s, MAX_PENDING))<0){
		perror("simplex-talk: listen"); exit(1);
	}

	printf("Welcome to the first TCP Server!\n");
	/* wait for connection, then receive and print text */
	addr_len = sizeof (client_addr);
	while(1) {
		if((new_s = accept(s, (struct sockaddr *)&client_addr, &addr_len)) < 0) {
			perror("simplex-talk: accept");
			exit(1);
		}
		printf("Connection established.\n");
		while (1){
			if((len=recv(new_s, buf, sizeof(buf), 0))==-1){
				perror("Server Received Error!");
				exit(1);
			}
			else if(len==0){
				printf("BUSTING OUT\n");
				break;
			}
			printf("TCP Server Received: %s", buf);
			complete_request(new_s, buf);
			bzero((char *) &buf, sizeof(buf));
		}

		printf("Client finishes, close the connection!\n");
		close(new_s);
	}

	close(s);
}

/* Complete the task that was requested */
void complete_request(int s, char buf[]){
	char *command = strtok(buf, " ");
	char *arg1 = strtok(NULL, " ");
	printf("Received command: %s with arg1: %s\n", command, arg1);
		
	char reply[BUFSIZ];
	if(strcmp(command, "LS\n") == 0){
		printf("We are in the LS case\n");
		ls(reply);
	} else if(strcmp(command, "MKDIR") == 0){
		printf("We are in the MKDIR case\n");
		mkdir(arg1, reply);
	} else if(strcmp(command, "RMDIR") == 0){
		printf("We are in the RMDIR case\n");
	} else if(strcmp(command, "DN") == 0){
		printf("We are in the DN case\n");
	} else if(strcmp(command, "UP") == 0){
		printf("We are in the UP case\n");
	} else {
		printf("Bad operation - not recognized\n");
	}
	send_fn(s, reply, sizeof(reply));
	printf(reply);
	bzero((char *) &reply, sizeof(reply));
}

void ls(char reply[]){
	FILE *fp = popen("ls -l", "r");
	if(fp == NULL){
		printf("LS error");
	}
	/* TODO - send the total size per directions */
	/*while(fgets(reply, BUFSIZ, fp) != NULL){
		printf(reply);
	} */
	fread(reply, sizeof(char), BUFSIZ, fp);
}

void mkdir(char *arg1, char reply[]){
	/* Check if the directory already exists */
	DIR* dir = opendir(arg1);
	if(dir){ // Dir already exists
		printf("Directory already exists");
		sprintf(reply, "-2");
		closedir(dir);
		return;
	}
		
	char buffer[BUFSIZ];
	sprintf(buffer, "mkdir %s", arg1);
	FILE *fp = popen(buffer, "r");
		
	if(fp == NULL){ // Error making dir
		printf("MKDIR error");
		sprintf(reply, "-2");
		return;
	}

	sprintf(reply, "1");
}

void send_fn(int socket, char buf[], int len){
	if(send(socket, buf, len, 0)==-1){
		printf("Server response error");
	}
}
