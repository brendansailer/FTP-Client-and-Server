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
void rmfile(int, char *, char*);
void cd(int, char *, char*);
void send_fn(int, char*);
void send_int(int, int);
int recv_int(int socket);
void recv_fn(int, char*);
void download(int, char *);
void upload(int, char *);
void ls(int, char*, char*);
void head(int, char*, char*);

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
		if(strcmp(command, "MKDIR") == 0){
			char *arg1 = strtok(NULL, " ");
			mkdir(s, arg1, command, reply);
			printf("End of mkdir\n");
		} else if(strcmp(command, "RMDIR") == 0){
			char *arg1 = strtok(NULL, " ");
			rmdir(s, arg1, command);
			printf("End of rmdir\n");
		} else if(strcmp(command, "RM") == 0){
			char *arg1 = strtok(NULL, " ");
			rmfile(s, arg1, command);
			printf("End of rm\n");
		} else if(strcmp(command, "CD") == 0){
			char *arg1 = strtok(NULL, " ");
			cd(s, arg1, command);
			printf("End of cd\n");
		} else if(strcmp(command, "LS\n") == 0){
				ls(s, command, reply);
		} else if(strcmp(command, "HEAD") == 0){
				char *arg1 = strtok(NULL, " ");
				head(s, command, arg1);
        } else if(strcmp(command, "DN") == 0){
            download(s, command);
	    } else if(strcmp(command, "UP") == 0){
            upload(s, command);
	    } else {
			printf("Unknown Operation\n");
			send_fn(s, command);
			send_int(s, 1234);
			printf("Got the int is: %d\n", recv_int(s));
		}
		
		bzero((char *)&buf, sizeof(buf));
		bzero((char *)&reply, sizeof(reply));
	}

	close(s); 
}

void head(int s, char* command, char* arg1){
	char buff[BUFSIZ];
	send_fn(s, command); //Sends the command to the server
	
	recv_fn(s, buff); 
	while(strcmp(buff, "-1") != 0){ // Loop over the LS data until -1 is recevied
		printf(buff);
		recv_fn(s, buff); // Get the size of the LS response
  }
}

void ls(int s, char* command, char* reply){
	char buff[BUFSIZ];
	send_fn(s, command); //Sends the command to the server
	
	recv_fn(s, buff); 
	while(strcmp(buff, "-1") != 0){ // Loop over the LS data until -1 is recevied
		printf(buff);
		recv_fn(s, buff); // Get the size of the LS response
  }
}

void rmfile(int s, char *arg1, char *command){
	char buf[BUFSIZ];
	char reply[BUFSIZ];

	// Check if no file was passed
	if(!strcmp(arg1, "") || !strcmp(arg1, "\n")){
		printf("Please pass a file name\n");
		return;
	}
	sprintf(buf, "%s %s", command, arg1);
	//send_int(s, strlen(arg1));
	send_fn(s, buf);
	
	// Read the reply
	int result = recv_int(s);
	if(result == -1){
		printf("The file does not exist on the server\n");
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
		send_fn(s, "Yes");
		int result = recv_int(s);
		if(result == 0){
			printf("File deleted\n");
		} else if(result == -1){
			printf("Faild to delete file\n");
		}
	} else { // Send No
		printf("Delete abandoned by user!\n");
		send_fn(s, "No");
	}

	// Consume the '\n' after the Yes/No confirmation
	fgets(buf, sizeof(buf), stdin);
}

void cd(int s, char *arg1, char *command){
	char buf[BUFSIZ];
	char reply[BUFSIZ];
	// Check if no file was passed
	if(!strcmp(arg1, "") || !strcmp(arg1, "\n")){
		printf("Please pass a valid directory name\n");
		return;
	}
	sprintf(buf, "%s %s", command, arg1);
	send_fn(s, buf);

	int result = recv_int(s);
	if(result == -2){
		printf("The directory does not exist on the server\n");
	} else if(result == -1){
		printf("Error in changing directory\n"); 
  } else{
		printf("Changed current directory\n");
	}
}

void mkdir(int s, char *arg1, char *command, char *reply){
	char buf[BUFSIZ];

	// Check if no directory was passed
	if(!strcmp(arg1, "") || !strcmp(arg1, "\n")){
		printf("Please pass a directory name\n");
		return;
	}
	sprintf(buf, "%s %s", command, arg1);
	send_fn(s, buf);

	int result = recv_int(s);
	if(result == -2){
		printf("The directory already exists on server\n");
	} else if(result == -1){
		printf("Error in making directory\n");
	} else{
		printf("The directory was successfully made\n");
	}
}

void rmdir(int s, char *arg1, char *command){
	char buf[BUFSIZ];
	char reply[BUFSIZ];

	// Check if no directory was passed
	if(!strcmp(arg1, "") || !strcmp(arg1, "\n")){
		printf("Please pass a directory name\n");
		return;
	}
	sprintf(buf, "%s %s", command, arg1);
	send_fn(s, buf);
	
	int response = recv_int(s);
	if(response == -2){
		printf("The directory is not empty\n");
		return;
	} else if(response == -1){
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
		send_fn(s, "Yes");
		
		response = recv_int(s);
		if(response == 1){
			printf("Directory deleted\n");
		} else if(response == -1){
			printf("Faild to delete directory\n");
		}
	} else { // Send No
		printf("Delete abandoned by user!\n");
		send_fn(s, "No");
	}

	// Consume the '\n' after the Yes/No confirmation
	fgets(buf, sizeof(buf), stdin);
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
		int length;

    FILE *wr = fopen(filename, "w+");

		if(wr == NULL){
			printf("CANNOT OPEN FILE\n");
		}

    //Writes the file to the disk portion by portion
    while(remaining > 0){
				if(remaining > BUFSIZ){
						length = BUFSIZ;
				} else {
						length = remaining;
				}

        //Gets the file data
        if((read = recv(socket, file_portion, length, 0))==-1){
		    	perror("myftp: error receiving reply");
	      }

				printf("remaining: %d read: %d\n", remaining, read);
        remaining -= read;

        //Writes the file to the disk
        fwrite(file_portion, sizeof(char), length, wr);
				bzero((char *)&file_portion, sizeof(file_portion));
		}

    fclose(wr);
		//recv_fn(socket, md5);

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
    bzero((char *)&md5_res, sizeof(md5_res));
    bzero((char *)&res, sizeof(res));
    bzero((char *)&md5, sizeof(md5));
    bzero((char *)&file_portion, sizeof(file_portion));
}

void upload(int socket, char *command){
    char *filename      = strtok(NULL, "\n"); //Filename itself
    int filename_length = strlen(filename);
    char client_command[BUFSIZ];
    char response[BUFSIZ];
    char file_buf[BUFSIZ];
    char md5[BUFSIZ];

    //If the file exists, send the command to the server
    FILE *file;
    int file_size;
    if ((file = fopen(filename, "r"))){
        //Creates the command to send to the server
        sprintf(client_command, "%s %d %s", command, filename_length, filename);
        send_fn(socket, client_command); //Sends the command to the server

        //Get the server response
	    if((recv(socket, response, sizeof(response), 0))==-1){
		    perror("myftp: error receiving reply");
	    }

        if(strcmp(response, "ok") != 0 ){
            perror("The server is not ready\n");
            return;
        }
        
        bzero((char *)&response, sizeof(response));

        //Finds the size of the file
        fseek(file, 0L, SEEK_END);
        long int remaining = ftell(file);
        file_size = remaining;
        rewind(file);
        sprintf(response, "%lu", remaining);

        //Send the file size
        send_fn(socket, response);

        usleep(1000);

        //Reads the entire file into a buffer
        while( remaining > 0){
            int count = fread(file_buf, sizeof(char), BUFSIZ, file);
            if( count < 0 ){
                perror("fread error");
                return;
            }
            remaining -= count;
            
            //send the current portion of the file
            send_fn(socket, file_buf);
        }

        fclose(file);

        
        //Computes the md5sum of the file
        char com[BUFSIZ];
        sprintf(com, "md5sum %s", filename);
        FILE* command_result = popen(com, "r");
	    fread(md5, sizeof(char), 32, command_result);
        printf("Calculated: %s\n", md5);

        //Gets the throughput from the server
        char throughput[BUFSIZ];
	    if((recv(socket, throughput, sizeof(throughput), 0))==-1){
		    perror("myftp: error receiving reply");
	    }
        
        //Display throughput information
        printf("%s\n", throughput);

        //Gets the md5sum from the server
	    if((recv(socket, response, sizeof(response), 0))==-1){
		    perror("myftp: error receiving reply");
	    }
        

        if(strcmp(response, md5) != 0 ){
            perror("md5sums do not match\n");
            return;
        }

        //Print matching message
        printf("MD5 hash: %s (matches)\n", response);
    
    } else {
        perror("File does not exist\n");
        return;
    }


    bzero((char *)&response, sizeof(response));
    bzero((char *)&file_buf, sizeof(file_buf));
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
}

void send_int(int socket, int value){
	int formatted = htonl(value);
	if(send(socket, &formatted, sizeof(formatted), 0)==-1){
		printf("Client send error");
	}
}

int recv_int(int socket){
	int length;
	int received_int;
	if((length=recv(socket, &received_int, sizeof(received_int), 0)) < 0){
		perror("myftp: error receiving reply");
		exit(1);
	}
	return ntohl(received_int);
}
