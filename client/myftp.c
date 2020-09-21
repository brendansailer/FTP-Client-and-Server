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
void mkdir(int, char *, char*, char*);
void rmdir(int, char *, char*);
void send_fn(int, char*);
void recv_fn(int, char*);
void download(int, char *);

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
		if (!strncmp(buf, "QUIT", 4)){
			printf("Good Bye!\n");
			break;
		}

		char *command = strtok(buf, " ");
		printf("Command: %s\n",command);
		if(strcmp(command, "MKDIR") == 0){
			char *arg1 = strtok(NULL, " ");
			mkdir(s, arg1, command, reply);
			printf("End of mkdir\n");
		} else if(strcmp(command, "RMDIR") == 0){
			char *arg1 = strtok(NULL, " ");
			rmdir(s, arg1, command);
			printf("End of rmdir\n");
		} else if(strcmp(command, "LS") == 0){
			send_fn(s, buf);
			recv_fn(s, reply);
			printf("%s", reply);
        } else if(strcmp(command, "DN") == 0){
            download(s, command);
            printf("End of download\n");
	    } else {
			printf("Unknown Operation\n");
		}
		
		bzero((char *)&buf, sizeof(buf));
		bzero((char *)&reply, sizeof(reply));
	}

	close(s); 
}

void mkdir(int s, char *arg1, char *command, char *reply){
	char buf[BUFSIZ];
	// Check if no directory was passed
	if(strcmp(arg1, "") == 0){
		printf("Please pass a directory name\n");
		return;
	}
	sprintf(buf, "%s %s", command, arg1);
	send_fn(s, buf);
	recv_fn(s, reply);
	printf("Got the reply: %s \n", reply);
	if(strcmp(reply, "-2") == 0){
		printf("The directory already exists on server\n");
	} else if(strcmp(reply, "-1") == 0){
		printf("Error in making directory\n");
	} else{
		printf("The directory was successfully made\n");
	}
}

void rmdir(int s, char *arg1, char *command){
	char buf[BUFSIZ];
	char reply[BUFSIZ];
	// Check if no directory was passed
	if(strcmp(arg1, "") == 0){
		printf("Please pass a directory name\n");
		return;
	}
	sprintf(buf, "%s %s", command, arg1);
	send_fn(s, buf);
	recv_fn(s, reply);
	printf("Got the reply: %s\n", reply);
	if(strcmp(reply, "-2") == 0){
		printf("The directory is not empty\n");
		return;
	} else if(strcmp(reply, "-1") == 0){
		printf("The directory does not exist on the server\n");
		return;
	} else{
		printf("Confirm delete? Yes/No: ");
	}
	bzero((char *)&buf, sizeof(buf));
	bzero((char *)&reply, sizeof(reply));
			
	// Ask the user for confirmation of delete
	char str[5];
	scanf("%s", str);
	if(strcmp(str, "Yes") == 0){ // Send Yes
		printf("Yes Case\n");
		sprintf(buf, "Yes");
		send_fn(s, buf);
		printf("Sent yes - waiting for reply\n");
		recv_fn(s, reply);
		printf("Got the reply: %s\n", reply);
		if(strcmp(reply, "1") == 0){
			printf("Directory deleted\n");
		} else if(strcmp(reply, "-1") == 0){
			printf("Faild to delete directory\n");
		}
	} else { // Send No
		printf("Delete abandoned by user!\n");
		sprintf(buf, "No");
		send_fn(s, buf);
	}
}

void download(int socket, char *command){
    char *filename      = strtok(NULL, "\n"); //Filename itself
    int filename_length = strlen(filename);
    char client_command[BUFSIZ];
    char md5[BUFSIZ];
    char file_size[BUFSIZ];
    char file_portion[BUFSIZ];

    sprintf(client_command, "%s %d %s", command, filename_length, filename);

    //Gets the time interval initial value
    struct timeval t0;
    gettimeofday(&t0, 0);

    send_fn(socket, client_command); //Sends the command to the server

	//Get the md5 hash response
	if((recv(socket, md5, sizeof(md5), 0))==-1){
		perror("myftp: error receiving reply");
	}

    //Server returns -1 if the file cannot be found
    if(strcmp(md5, "-1") == 0){
        printf("Server could not find the file %s\n", filename);
        bzero((char *)&md5, sizeof(md5));
        return;
    } 
                
    //Gets the size of the file
	if((recv(socket, file_size, sizeof(file_size), 0))==-1){
		perror("myftp: error receiving reply");
	}
    
    int remaining;
    sscanf(file_size, "%d", &remaining);
    int total_bytes = remaining;
    int read;

    FILE *wr = fopen(filename, "w");

    //Writes the file to the disk portion by portion
    while(remaining > 0){
        //Gets the file data
        if((read = recv(socket, file_portion, remaining, 0))==-1){
		    perror("myftp: error receiving reply");
	    }
        remaining -= read;

        //Writes the file to the disk
        fwrite(file_portion, sizeof(char), read, wr);
    }

    fclose(wr);

    //Gets the time interval initial value
    struct timeval t1;
    gettimeofday(&t1, 0);

    long elapsed = (t1.tv_sec - t0.tv_sec)*1000000  + (t1.tv_usec - t0.tv_usec);
    printf("%s bytes transferred in %f seconds: %f Megabytes/sec\n", file_size, elapsed/1000000., total_bytes/(float)elapsed);

    //Computes the md5 of the written file
    char res[BUFSIZ];
    char md5_res[BUFSIZ];
    sprintf(res, "md5sum %s", filename);
    FILE* command_result = popen(res, "r");
    fread(md5_res, sizeof(char), 32, command_result);

    if(strcmp(md5, md5_res) != 0){
        printf("File md5sums do not match\n");
    } else{
        printf("MD5 hash: %s (matches)\n", md5);
    }

    bzero((char *)&file_size, sizeof(file_size));
    bzero((char *)&md5, sizeof(md5));
}

void send_fn(int socket, char *buf){
	if(send(socket, buf, strlen(buf)+1, 0)==-1){
		printf("Client send error");
	}
}

void recv_fn(int socket, char *reply){
	int length;
	if((length=recv(socket, reply, sizeof(reply), 0))==-1){
		perror("myftp: error receiving reply");
		exit(1);
	}
	printf("The length is: %d\n", length);
}
