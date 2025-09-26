/* The code is subject to Purdue University copyright policies.
 * Do not share, distribute, or post online.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>

#define SERVPORT 53004 //no clue but its in the sample code from lab 0


int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "usage: ./http_client [host] [port number] [filepath]\n");
        exit(1);
    }

    char *host = argv[1];
    int port = atoi(argv[2]); //function to convert port number from string to int
    char *filepath = argv[3];

    //extract file name from filepath
    char *filename;
    
    int res = strcmp(filepath, "/"); 
    //if filepath is just '/', then filename should be index_html
    if (res == 0) 
    {
        filename = "index_html";
    } 
    
    else 
    {
        char *slash_symbol = strrchr(filepath, '/'); //returns pointer to last occurance of slash symbol in filepath
        if (slash_symbol != NULL) {
            filename = slash_symbol + 1; 
            //move pointer to next char after '/'
        }
        else //if no slash symbol in filepath, filename is the whole filepath
        {
            filename = filepath;
        }
    }

    //create socket 
    //from lab 0 sample code
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct hostent *he; //used for DNS lookup
    struct sockaddr_in their_addr; //server address

    /* get server's IP from server hostname by invoking the DNS */
	if ((he = gethostbyname(argv[1])) == NULL) {
		herror("gethostbyname"); //prints error if DNS lookup fails
		exit(1);
	}
    
    if (sockfd < 0) { //creates TCP socket
        perror("socket");
        exit(1);
    }


    their_addr.sin_family = AF_INET; //common address family from notes
	their_addr.sin_port = htons(port); //port from argv[2]
	their_addr.sin_addr = *((struct in_addr *)he->h_addr_list[0]); //server IP address
    //first ip address returned from the DNS lookup
	bzero(&(their_addr.sin_zero), 8); //rest of struct set to 0

    if (connect(sockfd, (struct sockaddr *) &their_addr, sizeof(struct sockaddr)) < 0) {
		perror("connect");
		exit(1);
	}

    //send HTTP GET request
    /*
    http request format:
    GET /path/file.html HTTP/1.0\r\n
    Host: <hostname>:<port>\r\n
    \r\n
    */

    char request[1024]; //buffer to hold http request
    snprintf(request, sizeof(request), "GET %s HTTP/1.0\r\nHost: %s:%d\r\n\r\n", filepath, host, port);
    //used snprintf to format the request string
    
    if(send(sockfd, request, strlen(request), 0) < 0) { 
        //send http request to server, 0 is for flags parameter
        perror("send");
        exit(1);
    }
    
    /*
    The response from the web server will look something like this:
    HTTP/1.0 200 OK\r\n
    Date: Fri, 31 Dec 2020 23:59:59 GMT\r\n
    Content-Type: text/html\r\n
    Content-Length: 1354\r\n
    [blank line]\r\n
    [file content]
    */
    
    int numbytes;
    char str[1000]; //buffer to hold response from server

    if ((numbytes = recv(sockfd, str, 1000, 0)) < 0) { /* numbytes is the actual number of bytes received.
                                                            * Note that numbytes may be smaller than the 
                                                            * str buffer size (1000 bytes in this case).
                                                            */
            perror("recv");
            exit(1);
        }
    str[numbytes] = '\0'; //null the received string
    
    //otherwise, print the response to stdout

    // printf("\n");if (strstr(str, "DONE") != NULL) //break;
    //     printf("%s", str);
    // }

	// close(sockfd);
    // return 0;
}

