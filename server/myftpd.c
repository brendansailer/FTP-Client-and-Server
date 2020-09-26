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
void ls(int, char *);
void head(int, char *, char *);
void mk_dir(int, char *, char *);
void rm_dir(int, char *, char*);
void rm_file(int, char *, char*);
void cd(int, char *, char*);
void send_fn(int, char*);
void recv_fn(int socket, char*);
int  recv_int(int socket);
void send_int(int, int);
void download(int, char *);
void upload(int, char *);

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
				break;
			}
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
    char *command = strtok(buf, " ");

    if(command == NULL){
        printf("GOT A NULL COMMAND\n");
				return;
	}
	
	char reply[BUFSIZ];
	if(strcmp(command, "LS\n") == 0){
		ls(s, reply);

	} else if(strcmp(command, "HEAD") == 0){
		char *arg1 = strtok(NULL, " \n");
		head(s, arg1, reply);

	} else if(strcmp(command, "MKDIR") == 0){
		char *arg1 = strtok(NULL, " \n");
		mk_dir(s, arg1, reply);

	} else if(strcmp(command, "RMDIR") == 0){
		char *arg1 = strtok(NULL, " \n");
		rm_dir(s, arg1, reply);
	
	} else if(strcmp(command, "CD") == 0){
		char *arg1 = strtok(NULL, " \n");
		cd(s, arg1, reply);

	} else if(strcmp(command, "RM") == 0){
		char *arg1 = strtok(NULL, " \n");
		rm_file(s, arg1, reply);
	
	} else if(strcmp(command, "DN") == 0){
	    char *arg1          = strtok(NULL, " ");
        int filename_length = atoi(arg1);
        char *filename      = strtok(NULL, "\n");

        if(strlen(filename) != filename_length){
            exit(1);
        }
        download(s, filename);

	} else if(strcmp(command, "UP") == 0){
	    char *arg1          = strtok(NULL, " ");
        int filename_length = atoi(arg1);
        char *filename      = strtok(NULL, "\n");
        
        if(strlen(filename) != filename_length){
            printf("Filename and filename length do not match");
            exit(1);
        }
        upload(s, filename);

	}
	bzero((char *) &buf, sizeof(buf));
	bzero((char *) &reply, sizeof(reply));
}

void rm_file(int s, char *arg1, char *reply){
	// Check if file exists 
	FILE *fp = fopen(arg1, "r");
	if(fp == NULL){
		printf("File does not exist\n");
		send_int(s, -1);
		return;
	}
	fclose(fp);
	
	// Confirm file exists
	send_int(s, 1);
	
	// Get confirmation of delete from client	
	char buffer[BUFSIZ];
	bzero((char *)&buffer, sizeof(buffer));
	recv_fn(s, buffer);

	if(strcmp(buffer, "Yes") == 0){ // Delete the file
		if(remove(arg1) >= 0){
			send_int(s, 1); // delete success
		} else {
			send_int(s, -1); // delete failed
		}
	}
}

void cd(int s, char *arg1, char *reply){
	struct stat path;
	if(stat(arg1, &path) < 0){
		send_int(s, -2); // Directory does not exist
	} else if(!S_ISDIR(path.st_mode)){
		send_int(s, -1); // Not a directory
	}
	
	if(chdir(arg1)){
		send_int(s, -1); // chdir failed
	} else {
		send_int(s, 1);
	}
}

void head(int s, char *arg1, char *reply){
	char file_buf[BUFSIZ];
	char cwd[BUFSIZ];
	char command[BUFSIZ];
	
	// Check if file exists
	FILE *fp = fopen(arg1, "r");
	if(fp == NULL){
		printf("File does not exist\n");
		send_fn(s, "-1");
		return;
	}
	fclose(fp);

	// Get the current working directory
	if(getcwd(cwd, sizeof(cwd)) == NULL){
		printf("CWD error\n");
		send_fn(s, "-1");
	}

	// Prepare the command for popen
	sprintf(command, "head %s/%s", cwd, arg1);

	fp = popen(command, "r");
	if(fp == NULL){
		printf("LS error\n");
		send_fn(s, "-1");
		return;
	}
	
	//send the current portion of the HEAD
	int count = fread(file_buf, sizeof(char), BUFSIZ, fp);
	while(count > 0){
		send_fn(s, file_buf);
	  	count = fread(file_buf, sizeof(char), BUFSIZ, fp);
	}
	
	// Signal the end of the transmission
	send_fn(s, "-1"); 

	bzero((char *)&file_buf, sizeof(file_buf));
	bzero((char *)&cwd, sizeof(cwd));
	bzero((char *)&command, sizeof(command));
}

void ls(int s, char *reply){
	char file_buf[BUFSIZ];
	char cwd[BUFSIZ];
	char command[BUFSIZ];

	// Get the current working directory
	if(getcwd(cwd, sizeof(cwd)) == NULL){
		printf("CWD error\n");
		send_fn(s, "-1");
	}

	// Prepare the command for popen
	sprintf(command, "ls -l %s", cwd);

	FILE *fp = popen(command, "r");
	if(fp == NULL){
		printf("LS error\n");
		send_fn(s, "-1");
		return;
	}
	
	//send the current portion of the LS
	int count = fread(file_buf, sizeof(char), BUFSIZ, fp);
	while(count > 0){
		send_fn(s, file_buf);
	  	count = fread(file_buf, sizeof(char), BUFSIZ, fp);
	}
	
	// Signal the end of the transmission
	send_fn(s, "-1"); 

	bzero((char *)&file_buf, sizeof(file_buf));
	bzero((char *)&cwd, sizeof(cwd));
	bzero((char *)&command, sizeof(command));
}

void mk_dir(int s, char *arg1, char *reply){
	/* Check if the directory already exists */
	DIR* dir = opendir(arg1);
	if(dir){ // Dir already exists
		printf("Directory already exists\n");
		send_int(s, -2);
		closedir(dir);
		return;
	}
	closedir(dir);
		
	char buffer[BUFSIZ];
	sprintf(buffer, "mkdir %s", arg1);
	FILE *fp = popen(buffer, "r");
	if(fp == NULL){ // Error making dir
		printf("MKDIR error\n");
		send_int(s, -1);
		return;
	}

	send_int(s, 1);
}

void rm_dir(int s, char *arg1, char *reply){
	/* Check if directory exists */
	DIR* dir = opendir(arg1);
	if(dir == NULL){
		printf("Directory does not exist\n");
		send_int(s, -1);
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
		send_int(s, 1); // empty
	} else {
		send_int(s, -2); // not empty
		return;
	}

	// Get confirmation of delete from client	
	bzero((char *)&buffer, sizeof(buffer));
	recv_fn(s, buffer);

	if(strcmp(buffer, "Yes") == 0){ // Delete the dir
		if(rmdir(arg1) >= 0){
			send_int(s, 1); // delete success
		} else {
			send_int(s, -1); // delete failed
		}
	}
}

void download(int s, char *filename){
	int length;
    FILE *file;
    char md5[BUFSIZ];
    char file_size[BUFSIZ];
    char file_buf[BUFSIZ];

    //Check the file existence
    if ((file = fopen(filename, "r"))){
        //Sends the md5sum
        char command[BUFSIZ];
        sprintf(command, "md5sum %s", filename);
        FILE* command_result = popen(command, "r");
	    fread(md5, sizeof(char), 32, command_result);

        send_fn(s, md5);

        //Sends the file size
        fseek(file, 0L, SEEK_END);
        long int remaining = ftell(file);
        rewind(file);
        sprintf(file_size, "%lu", remaining);

        send_fn(s, file_size);

        bzero((char *)&file_size, sizeof(file_size));
        usleep(1000);

        //Reads the entire file into a buffer
        while(remaining > 0){
			if(remaining > BUFSIZ){
				length = BUFSIZ;
			} else {
				length = remaining;
			}

            int count = fread(file_buf, sizeof(char), length, file);
            if(count < 0){
                perror("fread error");
                return;
            }
            remaining -= count;

            //send the current portion of the file
			if(send(s, file_buf, length, 0)==-1){
				printf("Server response error");
				return;
			}

    		bzero((char *)&file_buf, sizeof(file_buf));
        }

    } else{ //If file doesnt exist return -1
        sprintf(md5, "-1");
        send_fn(s, md5);
    }

    bzero((char *)&md5, sizeof(md5));
}

void upload(int s, char *filename){
    char response[BUFSIZ];
    char file_portion[BUFSIZ];

    //Server responds with ok when ready
    sprintf(response, "ok");
    send_fn(s, response);

    bzero((char *)&response, sizeof(response));

    //Server gets the file size from the client
    if((recv(s, response, sizeof(response), 0)) == -1){
        perror("error receiving reply\n");
        return;
    }

    int remaining, written;
    sscanf(response, "%d", &remaining);
    FILE *wr = fopen(filename, "w+");
    int file_size = remaining;
	int length;

    //Gets the time interval initial value
    struct timeval t0;
    gettimeofday(&t0, 0);

    //Writes the file to the disk portion by portion
    while( remaining > 0 ){
		if(remaining > BUFSIZ){
			length = BUFSIZ;
		} else {
			length = remaining;
		}

        if((written = recv(s, file_portion, length, 0)) == -1){
            perror("error receiving reply\n");
			return;
        }

        remaining -= written;

        //Writes the file to the disk and zeroes the buffer
        fwrite(file_portion, sizeof(char), written, wr);
		bzero((char *)&file_portion, sizeof(file_portion));
    }

    fclose(wr);

    bzero((char *)&response, sizeof(response));
    
    //Gets the time interval final value
    struct timeval t1;
    gettimeofday(&t1, 0);
    
    //Computes the throughput
    long elapsed = (t1.tv_sec - t0.tv_sec)*1000000 + (t1.tv_usec - t0.tv_usec);
    float throughput = file_size/(float)elapsed;

    //Server sends the throughput
    sprintf(response, "%d bytes transferred in %f seconds: %f Megabytes/sec", file_size, elapsed/1000000., throughput);
    send_fn(s, response);
    bzero((char *)&response, sizeof(response));

    //Computes the md5sum of the file
    char command[BUFSIZ];
    sprintf(command, "md5sum %s", filename);
    FILE* command_result = popen(command, "r");
	fread(response, sizeof(char), 32, command_result);

    //Server sends the md5sum
    send_fn(s, response);
    
    bzero((char *)&file_portion, sizeof(file_portion));
    bzero((char *)&response, sizeof(response));
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

void send_int(int socket, int value){
	int formatted = htonl(value);
	if(send(socket, &formatted, sizeof(formatted), 0)==-1){
		printf("Server response error");
	}
}

int recv_int(int socket){
	int len;
	int received_int;
	if((len=recv(socket, &received_int, sizeof(received_int), 0)) < 0){
			perror("Server Received Error!");
			return -1;
	}
	return ntohl(received_int);
}
