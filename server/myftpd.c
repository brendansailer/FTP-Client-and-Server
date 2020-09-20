// Programming Assignment 2
// Brendan Sailer (bsailer1)
// Brennen Hogan  (bhogan1)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <dirent.h>
#include <unistd.h>
#define MAX_PENDING 5
#define MAX_LINE 4096

void server(int);
void complete_request(int, char *);
void ls(char *);
void mk_dir(char *, char *);
void rm_dir(int, char *, char*);
void send_fn(int, char*);
void recv_fn(int socket, char*);
void download(int, char *);

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
	int opt = 1;
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
void complete_request(int s, char * buf){
	printf("Initial buffer: %s", buf);
    char *command = strtok(buf, " ");
	
	char reply[BUFSIZ];
	if(strcmp(command, "LS\n") == 0){
		printf("We are in the LS case\n");
		ls(reply);
		send_fn(s, reply);
		printf("ls replying with: %s\n", reply);

	} else if(strcmp(command, "MKDIR") == 0){
		printf("We are in the MKDIR case\n");
		char *arg1 = strtok(NULL, " \n");
		mk_dir(arg1, reply);
		send_fn(s, reply);
		printf("mkdir replying with: %s\n", reply);

	} else if(strcmp(command, "RMDIR") == 0){
		printf("We are in the RMDIR case\n");
		char *arg1 = strtok(NULL, " \n");
		rm_dir(s, arg1, reply);

	} else if(strcmp(command, "DN") == 0){
	    char *arg1          = strtok(NULL, " ");
	    printf("Received command: %s with arg1: %s\n", command, arg1);
        int filename_length = atoi(arg1);
        char *filename      = strtok(NULL, "\n");

        if(strlen(filename) != filename_length){
            printf("Filename and filename length do not match");
            exit(1);
        }

		printf("We are in the DN case\n");
        download(s, filename);
	} else if(strcmp(command, "UP") == 0){
		printf("We are in the UP case\n");
	} else {
		printf("Bad operation - not recognized\n");
	}
	bzero((char *) &buf, sizeof(buf));
	bzero((char *) &reply, sizeof(reply));
}

void ls(char *reply){
	FILE *fp = popen("ls -l", "r");
	if(fp == NULL){
		printf("LS error\n");
	}
	/* TODO - send the total size per directions */
	/*while(fgets(reply, BUFSIZ, fp) != NULL){
		printf(reply);
	} */
	fread(reply, sizeof(char), BUFSIZ, fp);
}

void mk_dir(char *arg1, char *reply){
	/* Check if the directory already exists */
	printf("Directory Name: %s\n", arg1);
	DIR* dir = opendir(arg1);
	if(dir){ // Dir already exists
		printf("Directory already exists\n");
		sprintf(reply, "-2");
		closedir(dir);
		return;
	}
	closedir(dir);
		
	char buffer[BUFSIZ];
	sprintf(buffer, "mkdir %s", arg1);
	FILE *fp = popen(buffer, "r");
		
	if(fp == NULL){ // Error making dir
		printf("MKDIR error\n");
		sprintf(reply, "-1");
		return;
	}

	sprintf(reply, "1");
}

void rm_dir(int s, char *arg1, char *reply){
	/* Check if directory exists */
	char dest[100] = {"./"};
	strcat(dest, arg1);
	DIR* dir = opendir(dest);
	if(dir == NULL){
		sprintf(reply, "-1");
		send_fn(s, reply);
		printf("Directory does not exist\n");
		closedir(dir);
		return;
	}
	closedir(dir);

	/* Check if the directory is empty */
	char buffer[BUFSIZ];
	sprintf(buffer, "ls %s", arg1);
	FILE *fp = popen(buffer, "r");
	if(fp == NULL){
		printf("LS error\n");
	}
	fread(reply, sizeof(char), BUFSIZ, fp);
	if(strcmp(reply, "") == 0){
		sprintf(reply, "1"); // empty
	} else {
		sprintf(reply, "-2"); // not empty
	}
	printf("Reply sent\n");
	send_fn(s, reply);

	// Get confirmation of delete from client	
	bzero((char *)&buffer, sizeof(buffer));
	recv_fn(s, buffer);
	printf("Got the confirmation: %s\n", buffer);

	if(strcmp(buffer, "Yes") == 0){
		printf("delete the dir\n");
		// Delete the dir
		if(rmdir(arg1) >= 0){
			sprintf(reply, "1"); // delete success
		} else {
			sprintf(reply, "-1"); // delete failed
		}
		printf("rmdir replying with: %s\n", reply);
		send_fn(s, reply);
	} else { // Don't delete the directory
		printf("do nothing\n");
	}
}

void download(int s, char *filename){
    printf("Download %s\n", filename);
    FILE *file;
    char md5[BUFSIZ];
    char file_size[BUFSIZ];

    //Check the file existence
    if ((file = fopen(filename, "r"))){
        //Sends the md5sum
        char command[BUFSIZ];
        sprintf(command, "md5sum %s", filename);
        FILE* command_result = popen(command, "r");
	    fread(md5, sizeof(char), 32, command_result);

        printf("md5sum buf: %s\n", md5);
        
        send_fn(s, md5);

        //Sends the file size
        fseek(file, 0L, SEEK_END);
        long int res = ftell(file);
        rewind(file);
        sprintf(file_size, "%lu", res);
        printf("file size buf: %s", file_size);

        send_fn(s, file_size);

        bzero((char *)&file_size, sizeof(file_size));

    } else{ //If file doesnt exist return -1
        sprintf(md5, "-1");
        printf("Returning: %s\n", md5);
        send_fn(s, md5);
    }
    bzero((char *)&md5, sizeof(md5));
}

void send_fn(int socket, char *buf){
	if(send(socket, buf, strlen(buf)+1, 0)==-1){
		printf("Server response error");
	}
}

void recv_fn(int socket, char *buf){
	int len;
	if((len=recv(socket, buf, sizeof(buf), 0))==-1){
			perror("Server Received Error!");
	}
}
