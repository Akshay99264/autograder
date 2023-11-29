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
    if (argc != 7)
    {
        perror("./submit  <serverIP:port>  <sourceCodeFileTobeGraded>  <loopNum> <sleepTimeSeconds> <timeout-seconds>\n");
        return -1;
    }

    char server_ip[40], ip_port[40], file_path[256];
    int server_port, loop_num, sleep_time, successfull_requests, timeout_count = 0, error_count = 0;
    double total_response_time = 0, throughput = 0, avg_response_time = 0, request_rate = 0, goodput = 0, timeout_rate = 0, error_rate = 0;
    struct timeval start, end, loop_start, loop_end;

    strcpy(server_ip, argv[1]);
    server_port = atoi(argv[2]);
    strcpy(file_path, argv[3]);
    loop_num = atoi(argv[4]);
    successfull_requests = loop_num;
    sleep_time = atoi(argv[5]);
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

    struct timeval timeout;
    timeout.tv_sec = atoi(argv[6]);
    timeout.tv_usec = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
        printf("timeout");

    gettimeofday(&loop_start, NULL);
    for (int i = 0; i < loop_num; i++)
    {
        gettimeofday(&start, NULL);
        bool error_flag = false;
        if (send_file(sockfd, file_path) != 0)
        {
            printf("Error sending source file\n");
            successfull_requests--;
            error_count++;
            error_flag = true;
        }

        int bytes_read;
        char buffer[BUFFER_SIZE];
        bytes_read = read(sockfd, buffer, sizeof(buffer));
        if (bytes_read < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                printf("Timeout occured\n");
                successfull_requests--;
                timeout_count++;
                flag = 0;
            }
            else
            {
                printf("ERROR reading from socket");
                if (!error_flag)
                {
                    error_count++;
                    successfull_requests--;
                    flag = 0;
                }
            }
        }
        if (flag)
        {
            buffer[bytes_read] = '\0';
            write(STDOUT_FILENO, "Server Response: ", 17);
            write(STDOUT_FILENO, buffer, bytes_read);
        }

        printf("\n");
        gettimeofday(&end, NULL);

        if (i != loop_num)
            sleep(sleep_time);
        double response_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
        total_response_time = total_response_time + response_time;
    }

    gettimeofday(&loop_end, NULL);
    double loop_time = (loop_end.tv_sec - loop_start.tv_sec) + (loop_end.tv_usec - loop_start.tv_usec) / 1e6;
    avg_response_time = total_response_time / loop_num;
    throughput = successfull_requests / loop_time;
    goodput = successfull_requests / total_response_time;
    timeout_rate = timeout_count / loop_time;
    error_rate = error_count / loop_time;
    request_rate = loop_num / loop_time;

    // <Average response time>, <Throughput>, <Goodput>, <Timeout rate>, <Error Rate>, <Request Rate>, <Successfull requests>, <Loop time>
    printf("%f,%f,%f,%f,%f,%f,%d,%f\n", avg_response_time, throughput, goodput, timeout_rate, error_rate, request_rate, successfull_requests, loop_time);

    close(sockfd);
    return 0;
}
