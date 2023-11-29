#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <error.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <errno.h>

const int BUFFER_SIZE = 5000;
const int MAX_FILE_SIZE_BYTES = 4;
const int MAX_TRIES = 5;

// Function to send file
int send_file(int sockfd, char *file_path)
{
    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);
    FILE *file = fopen(file_path, "rb");

    if (!file)
    {
        printf("Error opening file");
        return -1;
    }

    fseek(file, 0L, SEEK_END);
    int file_size = ftell(file);
    fseek(file, 0L, SEEK_SET);
    char file_size_bytes[MAX_FILE_SIZE_BYTES];
    memcpy(file_size_bytes, &file_size, sizeof(file_size));
    // Send file size to server
    if (send(sockfd, &file_size_bytes, sizeof(file_size_bytes), 0) == -1)
    {
        printf("Error sending file size");
        fclose(file);
        return -1;
    }

    while (!feof(file))
    {
        size_t bytes_read = fread(buffer, 1, BUFFER_SIZE - 1, file);

        // Send file to server
        if (send(sockfd, buffer, bytes_read + 1, 0) == -1)
        {
            printf("Error sending file data");
            fclose(file);
            return -1;
        }

        memset(buffer, 0, sizeof(buffer));
    }

    fclose(file);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        perror("./submit <new|status> <serverIP> <port>  <sourceCodeFileTobeGraded|requestID>\n");
        return -1;
    }

    char server_ip[40], ip_port[40], file_path[256], requestID[38], request_type[10];
    int server_port, loop_num, sleep_time, successfull_requests, timeout_count = 0, error_count = 0, request_flag = 0;
    double total_response_time = 0, throughput = 0, avg_response_time = 0, request_rate = 0, goodput = 0, timeout_rate = 0, error_rate = 0;
    struct timeval start, end, loop_start, loop_end;

    strcpy(request_type,argv[1]);
    strcpy(server_ip, argv[2]);
    server_port = atoi(argv[3]);

    if (strcmp(request_type, "status") == 0 || strcmp(request_type, "Status") == 0)
    {
        request_flag = 0;
        strcpy(requestID, argv[4]);      
    }
    else if (strcmp(request_type, "new") == 0 || strcmp(request_type, "New") == 0)
    {
        request_flag = 1;
        strcpy(file_path, argv[4]);   
    }
    else
    {
        perror("./submit <new | status>  <serverIP> <port>  <sourceCodeFileTobeGraded | requestID>\n");
        return -1;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1)
    {
        perror("Socket creation failed");
        return -1;
    }

    struct sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &serv_addr.sin_addr.s_addr);
    int tries = 0, flag = 1;

    while (true)
    {
        if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == 0)
            break;

        sleep(1);
        tries += 1;

        if (tries == MAX_TRIES)
        {
            printf("Server not responding\n");
            return -1;
        }
    }

    int n=send(sockfd,request_type,sizeof(request_type),0);
    if(n<0)
    {
        perror("ERROR sending request type");
        close(sockfd);
        return -1;
    }
    if(request_flag == 1)
    {
        
        if (send_file(sockfd, file_path) != 0)
        {
            printf("Error sending source file\n");
            return 0;
        }
    }
    else
    {
        // code to handle status case;
    }

    int bytes_read;
    char buffer[BUFFER_SIZE];
    bytes_read = read(sockfd, buffer, sizeof(buffer));
    if (bytes_read < 0)
    {
        printf("ERROR reading from socket");
        return 0;
    }

    buffer[bytes_read] = '\0';
    write(STDOUT_FILENO, "Server Response: ", 17);
    write(STDOUT_FILENO, buffer, bytes_read);

    printf("\n");
    close(sockfd);
    return 0;
}
