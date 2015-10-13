
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <ctype.h>

//fix to connect on port designated in cmmd line

ssize_t lookup_address(char *host, struct sockaddr **addr, char *port)
{
    	struct addrinfo hints = {
    	    0, AF_INET, SOCK_DGRAM, 0
    	};
    	struct addrinfo *ai;
    	ssize_t len;
    	int code;
    	code = getaddrinfo(host, port, &hints, &ai);
    	if (code < 0) {
        	return -1;
    	}

    // now we copy the socket address
    	len = ai->ai_addrlen;
    	*addr = malloc(len);
    	memcpy(*addr, ai->ai_addr, len);
    	freeaddrinfo(ai);
	return len;
}

char * strAdd(char *strOne, char *strTwo){
        char * returnStr = (char *) malloc(1+ strlen(strOne) + strlen(strTwo));
        strcpy(returnStr, strOne);
        strcat(returnStr, strTwo);
        return returnStr;
}


int main(int argc, char *argv[]){
	
	if(argc == 0){
		perror("Need URL\n");
		exit(EX_USAGE);
	}else if(strstr(argv[1], "http://") == NULL){
		perror("...httpd://\n");
		exit(EX_USAGE);
	}

	struct sockaddr *addr;
	int code = 0;
	ssize_t addr_len;
	char *nohttp = NULL;
	char *host = NULL;
	char *strtoremove = "http://";
	char *locationport = NULL; 
	char *locationpath = NULL;
	char *path = NULL;
	long port = 0;
	nohttp = argv[1];
	host = strstr(nohttp, strtoremove);	
	if(host != NULL){
		strcpy(host, host + strlen(strtoremove));

		//check port
		locationport = strchr(host, ':');
		if(locationport != NULL){
			//there is a port number.
			while(*locationport){
				if(isdigit(*locationport)){
					port = strtol(locationport, &locationport, 10);	
				}else{
					locationport++;
				}
			}
		}
	
		//check path
		locationpath = strstr(host, "/");
		path = locationpath;
	}
	if(path == NULL){
		path = "/";
	}

	char realpath[1024];
	strcpy(realpath, path);
	
	if(port == 0){
                port = 80;
        }

	char portch[5];
	sprintf(portch, "%lu", port);
//	fprintf(stderr, "host: %s\n", host);
	host = strtok(host, ":");
	host = strtok(host, "/");
//	fprintf(stderr, "heyhost: %s\n%s\n", host, portch);
	
	addr_len = lookup_address(host, &addr, portch);
	if (addr_len <= 0) {
        	perror(host);
        	exit(EXIT_FAILURE);
    	}

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		perror("socket open failure");
		exit(EXIT_FAILURE);
	}
	code = connect(sockfd, addr, addr_len);
    	if (code < 0) {
        	perror("connect");
        	exit(EXIT_FAILURE);
    	}
//send request
//add :port back to host if not 80.
	if(port != 80){
		char *strTwo = strAdd(":", portch);
		char *tempstr = strAdd(host, strTwo);
		host = tempstr;
	}
	fprintf(stderr, ">GET %s HTTP/1.0\r\n", realpath);
	fprintf(stderr, ">Host: %s\r\n", host);
	fprintf(stderr, ">User-Agent: TXState CS4350 httpdump\r\n");
	fprintf(stderr, ">\r\n");
	
	char request[1024];
	char response[1024];
	sprintf(request, "GET %s HTTP/1.0\r\nHost:  %s\r\nUser-Agent: TXState CS4350 httpdump\r\n\r\n ", realpath, host);

	size_t requestlen = strlen(request);
	size_t bytes_done = 0;
	ssize_t result;
	

	while(bytes_done < requestlen){
		result = write(sockfd, request+bytes_done, requestlen - bytes_done);
		if(result < 0){
			exit(EX_IOERR);
		}
		bytes_done += result;
	}

	int count = 0;
	int statcode = 0;
	char *completeresponse = malloc(1024);
	char *token;
	char *status;
	while(1){
		result = read(sockfd, response, 1024);
		char *sep = response;
		token = strsep(&sep, "\r\n\r\n");
	
		count ++;
		if(result == NULL){
                        break;
                }
		if(count == 1){ 	
			fprintf(stderr, "<%s\n\n", token);
		//	fprintf(stderr, "\nsep: %s\n\n", sep);
			strcpy(completeresponse, response);
			status = malloc(sizeof(token));
			strcpy(status, token);	
		}else{
			completeresponse = strAdd(completeresponse, response);
		}
	
		ssize_t toprint = result;
		size_t bdone = 0;
		while(bdone < toprint){
			result = write(1, response + bdone, toprint - bdone);
			bdone += result;
		}
	}


	fprintf(stderr,"\n");

	while(*status){
                if(isdigit(*status)){
                        statcode = strtol(status, &status, 10);	
                        if(statcode >= 400){
                                exit(2);
                        }else if(statcode > 300 && statcode <=399){
                                exit(1);
                        }else if(statcode >= 100 && statcode <=300){
                        	exit(0);
                	}
                }else{
                	status++;
		}
	}
	close(sockfd);	
	free(addr);
	return 0;
}
