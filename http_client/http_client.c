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
#define HEADER_BUF 8192 //header buffer size 8KB

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
        filename = "index.html";
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
    snprintf(request, sizeof(request), "GET %s HTTP/1.1\r\nHost: %s:%d\r\n\r\n", filepath, host, port);
    //used snprintf to format the request string
    //used 1.1 instead of 1.0 because some servers dont support 1.0

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
    
    int numbytes; //number of bytes received
    char headerbuf[HEADER_BUF];
    int header_bytes = 0;
    char *body = NULL;

    //read headers until we find \r\n\r\n
    while (1) {
        numbytes = recv(sockfd, headerbuf + header_bytes, sizeof(headerbuf) - header_bytes - 1, 0);
        if (numbytes <= 0) {
            perror("recv");
            exit(1);
        }
        header_bytes += numbytes;
        headerbuf[header_bytes] = '\0';

        body = strstr(headerbuf, "\r\n\r\n");
        if (body) {
            body += 4; //move pointer to start of body
            break;
        }
    }

    //need to check for "200 OK" in the first line of the response
    char *first_line_end = strstr(headerbuf, "\r\n");
    if (first_line_end != NULL) 
    {
        *first_line_end = '\0';
        char *first_line = headerbuf;

        if (strncmp(first_line, "HTTP/1.0 200 OK", 15) != 0 && strncmp(first_line, "HTTP/1.1 200 OK", 15) != 0) 
        {
            printf("%s\n", first_line);
            close(sockfd);
            exit(0);
        }
        *first_line_end = '\r'; //restore the original string
    }

    //extract "Content-Length" from headers
    char *length_str = strstr(headerbuf, "Content-Length: ");
    int content_length = 0;
    if (length_str != NULL) {
        sscanf(length_str, "Content-Length: %d", &content_length);
    } else {
        printf("Content-Length not found\n");
        close(sockfd);
        exit(1);
    }

    //now we open output file
    FILE *fp = fopen(filename, "wb"); //open file in binary write mode
    //wb is for write binary
    if (fp == NULL) 
    {
        perror("fopen");
        close(sockfd);
        exit(1);
    }

    //write part of the body already in headerbuf to file
    int header_length = body - headerbuf; //length of header part
    int body_length = header_bytes - header_length; //length of body part already in buffer
    int total_received = 0;

    //write initial body part to file
    if (body_length > 0) {
        fwrite(body, 1, body_length, fp);
        total_received = body_length;
    }

    //continue receiving the rest of the file
    while (total_received < content_length) {
        char buffer[4096];  //use separate buffer
        numbytes = recv(sockfd, buffer, sizeof(buffer), 0); 
        if (numbytes <= 0) {
            break;
        }
        fwrite(buffer, 1, numbytes, fp);
        total_received += numbytes;
    }

    fclose(fp);
    close(sockfd);
    return 0;
}