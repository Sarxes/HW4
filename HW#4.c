#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <string.h>
#include <netinet/in.h> 
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define buffer_len 65536
#define path_len 256
#define listen_port 8000

// make root directory global
char *root_dir;

// make struct for client details passed to thread
typedef struct {
	int client_fd;
	char buffer[buffer_len];
} client_info;

// helper function to check if path is a file
int isFile(const char *path){
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

// helper function that reads file, returns 0 on success, 1 on fail
int readFile(const char *path, char *buffer){

	// try to open path; check open success and if path is a file
	int file = open(path, O_RDONLY);
	if (file < 0 || !isFile(path)) return 1;

	// clear buffer; read from file and return success
	memset(buffer, '\0', buffer_len);
	int bytes_read = read(file, buffer, buffer_len);
	return 0;
}

// threads use this function to handle clients
void * handleClient(void *info_void){

	// cast info pointer
	client_info *info = (client_info*) info_void;

	// recv / put request into struct
	recv(info->client_fd, info->buffer, buffer_len-1, 0);
    //read(info->client_fd, info->buffer, 1000);
    printf("%s\n", info->buffer);
    
	// parse the desired path, then reset buffer for reuse
	const char *sub_path = strtok((info->buffer) + (sizeof(char) * 4), " ");
	if (sub_path == NULL) return NULL;

	// make the full path and response
	char path[path_len], response[buffer_len];
	strcpy(path, root_dir);
	strcat(path, sub_path);

	// if read file failed show 404 page, else show read content
	if (readFile(path, info->buffer)) strcpy(response, "HTTP/1.0 404 Not Found\nContent-Length:358\n\n<head><title>webserver</title></head><body style=\"background: repeating-linear-gradient(-45deg, white, white 50px, black 50px, black 100px); display: flex; justify-content: center; align-items: center; width: 100%; height: 100%; color: black;\"><h1 style=\"background: white; padding: 20px; font-size: 500%; border: 8px solid black;\">404: Not Found</h1></body>");
	else{

		// reuse path for Content-Length
		sprintf(path, "%lu", strlen(info->buffer));

		// build response
		strcpy(response, "HTTP/1.0 200 OK\nContent-Length:");
		strcat(response, path);
		strcat(response, "\n\n");
		strcat(response, info->buffer);
	}
	
	// send client our response
	send(info->client_fd, response, strlen(response), 0);

	// free info and return
	free(info_void);
	return 0;
}

// main function
int main(int argc, char const *argv[]){
    
    char buffer[1000];
    
	// check vars
	if (argc != 2){
		printf("%s\n", "Please pass the home directory.");
		return 1;
	}

	// init global root diectory
	root_dir = (char*) malloc(sizeof(char) * path_len);
	strcpy(root_dir, argv[1]);

	// declare sockets
	int socket_fd, client_fd;
	struct sockaddr_in address;
	int addrlen = sizeof(address);

	// init socket
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd < 0) {
		printf("%s\n", "Error making socket.");
		return 1;
	}

	// specify address info
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY; 
	address.sin_port = htons(listen_port); 

	// bind to socket and listen on it with backlog 5
	bind(socket_fd, (struct sockaddr *) &address, sizeof(address));
	listen(socket_fd, 5);

	// make pthread_t for thread
	pthread_t thread;

	// loop until the heat death of the universe
	while(1){

		// allocate mem for request; accept a connection;
		client_info *info = (client_info*) malloc(sizeof(client_info));
		info->client_fd = accept(socket_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
 
		// pass client to thread
		pthread_create(&thread, NULL, handleClient, info);
		printf("%s\n", "Client passed to thread, listening...");
	}

	// close socket, free dir and return
	close(socket_fd);
	free(root_dir);
	return 0;
}