/* run using ./server <port> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <assert.h>
#include <pthread.h>
#include <sys/queue.h>
#define QUEUE_SIZE 30
void *thread_function(void *arg);
void enqueue(int *client_sockfd);
void masterFunc(int *pClient);
int *dequeue();
const int BUFFER_SIZE = 1024;
const int MAX_FILE_SIZE_BYTES = 4;
int front = 0, rear = 0, count = 0, found = 0, taskCount = 0;
int *queue[QUEUE_SIZE];
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t task_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t taskReady = PTHREAD_COND_INITIALIZER;

void error(char *msg)
{
	perror(msg);
	exit(1);
}

int recv_file(int sockfd, char *file_path)
{
    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);
    FILE *file = fopen(file_path, "wb");

    if (!file)
    {
        perror("Error opening file");
        return -1;
    }

    char file_size_bytes[MAX_FILE_SIZE_BYTES];

    if (recv(sockfd, file_size_bytes, sizeof(file_size_bytes), 0) <= 0)
    {
        // perror("Error receiving file size");
        fclose(file);
        return -1;
    }

    int file_size;
    memcpy(&file_size, file_size_bytes, sizeof(file_size_bytes));
    size_t bytes_read = 0, total_bytes_read = 0;

    while (true)
    {
        bytes_read = recv(sockfd, buffer, BUFFER_SIZE, 0);
        total_bytes_read += bytes_read;

        if (bytes_read <= 0)
        {
            perror("Error receiving file data");
            fclose(file);
            return -1;
        }

        fwrite(buffer, 1, bytes_read, file);
        bzero(buffer, BUFFER_SIZE);

        if (total_bytes_read >= file_size)
            break;
    }

    fclose(file);
    return 0;
}

char *compile_command(char *request_id, char *programFile, char *execFile)
{
	char *s;
	s = malloc(200 * sizeof(char));
	memset(s, 0, sizeof(s));
	strcpy(s, "gcc -o ");
	strcat(s, execFile);
	strcat(s, "  ");
	strcat(s, programFile);
	strcat(s, " 2> output/compiler_err");
	strcat(s, request_id);
	strcat(s, ".txt");
	return s;
}

char *run_command(char *request_id, char *execFileName)
{
	char *s;
	s = malloc(200 * sizeof(char));
	memset(s, 0, sizeof(s));
	strcpy(s, "./");
	strcat(s, execFileName);
	strcat(s, " > output/out");
	strcat(s, request_id);
	strcat(s, ".txt");
	strcat(s, " 2> output/runtime_err");
	strcat(s, request_id);
	strcat(s, ".txt");
	return s;
}

char *output_diff_command(char *request_id)
{
	char *s;
	s = malloc(200 * sizeof(char));
	memset(s, 0, sizeof(s));
	strcpy(s, "diff output/out");
	strcat(s, request_id);
	strcat(s, ".txt");
	strcat(s, " actualOutput.txt 1> output/diff_err");
	strcat(s, request_id);
	strcat(s, ".txt");
	return s;
}

char *makeProgramFileName(char *request_id)
{
	char *s;
	s = malloc(200 * sizeof(char));
	memset(s, 0, sizeof(s));
	strcpy(s, "files/file");
	strcat(s, request_id);
	strcat(s, ".c");
	return s;
}

char *makeCompileErrorFileName(char *request_id)
{
	char *s;
	s = malloc(200 * sizeof(char));
	memset(s, 0, sizeof(s));
	strcpy(s, "output/compiler_err");
	strcat(s, request_id);
	strcat(s, ".txt");
	return s;
}

char *makeRuntimeErrorFileName(char *request_id)
{
	char *s;
	s = malloc(200 * sizeof(char));
	memset(s, 0, sizeof(s));
	strcpy(s, "output/runtime_err");
	strcat(s, request_id);
	strcat(s, ".txt");
	return s;
}

char *makeOutputFileName(char *request_id)
{
	char *s;
	s = malloc(200 * sizeof(char));
	memset(s, 0, sizeof(s));
	strcpy(s, "files/out");
	strcat(s, request_id);
	strcat(s, ".txt");
	return s;
}

char *makeExecFileName(char *request_id)
{

	char *s;
	s = malloc(200 * sizeof(char));
	memset(s, 0, sizeof(s));
	strcpy(s, "output/prog");
	strcat(s, request_id);
	return s;
}

char *makeOutputDiffFileName(char *request_id)
{

	char *s;
	s = malloc(200 * sizeof(char));
	memset(s, 0, sizeof(s));
	strcpy(s, "files/diff_err");
	strcat(s, request_id);
	strcat(s, ".txt");
	return s;
}

void *grader(int *sockfd)
{
    int newsockfd = *(int *)sockfd;
    int n;
    char *programFile, *execFile, *compileErrorFile, *runtimeErrorFile, *outputFile, *outputDiffFile;
    programFile = makeProgramFileName((int)pthread_self());
    execFile = makeExecFileName((int)pthread_self());
    compileErrorFile = makeCompileErrorFileName((int)pthread_self());
    runtimeErrorFile = makeRuntimeErrorFileName((int)pthread_self());
    outputFile = makeOutputFileName((int)pthread_self());
    outputDiffFile = makeOutputDiffFileName((int)pthread_self());

    char *comp_command, *r_command, *diff_command;
    comp_command = compile_command((int)pthread_self(), programFile, execFile);
    r_command = run_command((int)pthread_self(), execFile);
    diff_command = output_diff_command((int)pthread_self());

    while (1)
    {
        if (recv_file(newsockfd, programFile) != 0)
        {
            close(newsockfd);
            return 0;
        }

        if (system(comp_command) != 0)
        {
            // Handle compiler error
            FILE *f = fopen(compileErrorFile, "rb");
            fseek(f, 0, SEEK_END);
            int len = ftell(f);
            rewind(f);
            char tempCompilerErrorBuffer[len];
            size_t bytes_read = fread(tempCompilerErrorBuffer, 1, sizeof(tempCompilerErrorBuffer), f);
            tempCompilerErrorBuffer[bytes_read] = '\0';
            char compilerErrorBuffer[bytes_read + 16];
            memset(compilerErrorBuffer, 0, sizeof(compilerErrorBuffer));
            strcat(compilerErrorBuffer, "COMPILER ERROR\n");
            strcat(compilerErrorBuffer, tempCompilerErrorBuffer);
            
            n = send(newsockfd, compilerErrorBuffer, sizeof(compilerErrorBuffer), 0);
            if (n < 0)
                error("ERROR writing to socket");
			
			fclose(f);
        }
        else if (system(r_command) != 0)
        {
            // Handle runtime time error
            FILE *f = fopen(runtimeErrorFile, "r");
            fseek(f, 0, SEEK_END);
            int len = ftell(f);
            rewind(f);
            char tempRuntimeErrorBuffer[len];
            size_t bytes_read = fread(tempRuntimeErrorBuffer, 1, sizeof(tempRuntimeErrorBuffer), f);
            tempRuntimeErrorBuffer[bytes_read] = '\0';
            char runtimeErrorBuffer[bytes_read + 15];
            memset(runtimeErrorBuffer, 0, sizeof(runtimeErrorBuffer));
            strcat(runtimeErrorBuffer, "RUNTIME ERROR\n");
            strcat(runtimeErrorBuffer, tempRuntimeErrorBuffer);

            n = send(newsockfd, runtimeErrorBuffer, sizeof(runtimeErrorBuffer), 0);
            if (n < 0)
                error("ERROR writing to socket");
			
			fclose(f);
        }
        else if (system(diff_command) != 0)
        {
            // Handle output difference
            FILE *f = fopen(outputDiffFile, "r");
            fseek(f, 0, SEEK_END);
            int len = ftell(f);
            rewind(f);
            char tempDiffErrorBuffer[len];
            size_t bytes_read = fread(tempDiffErrorBuffer, 1, sizeof(tempDiffErrorBuffer), f);
            tempDiffErrorBuffer[bytes_read] = '\0';
            char diffErrorBuffer[bytes_read + 18];
            memset(diffErrorBuffer, 0, sizeof(diffErrorBuffer));
            strcat(diffErrorBuffer, "OUTPUT DIFFRENCE\n");
            strcat(diffErrorBuffer, tempDiffErrorBuffer);
            
            n = send(newsockfd, diffErrorBuffer, sizeof(diffErrorBuffer), 0);
            if (n < 0)
                error("ERROR writing to socket");
			
			fclose(f);
        }
        else
        {
            // Send success message
            n = send(newsockfd, "PASS", sizeof("PASS"), 0);
            if (n < 0)
                error("ERROR writing to socket");
        }
    }
    free(programFile);
    free(execFile);
    free(comp_command);
    free(r_command);
    close(newsockfd);
}

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno;
	socklen_t clilen;
	char buffer[256];
	struct sockaddr_in serv_addr, cli_addr;
	int n;

	if (argc < 3)
	{
		fprintf(stderr, "Usage: %s <port no> <number of threads>\n", argv[0]);
		exit(1);
	}
	
	int MAX_THREADS = atoi(argv[2]);
	pthread_t threadpool[MAX_THREADS];
	
	for (int i = 0; i < MAX_THREADS; i++)
	{
		pthread_create(&threadpool[i], NULL, thread_function, NULL);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0)
		error("ERROR opening socket");


	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	portno = atoi(argv[1]);
	serv_addr.sin_port = htons(portno);

	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR on binding");

	listen(sockfd, 1000);
	clilen = sizeof(cli_addr);
	int reqID = 0;

	while (1)
	{
		newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
		if (newsockfd < 0)
			error("ERROR on accept");
		int *pClient = malloc(sizeof(int));
		*pClient = newsockfd;
		masterFunc(pClient);
	}

	pthread_mutex_destroy(&queue_mutex);
	pthread_cond_destroy(&taskReady);

	close(sockfd);
	return 0;
}

void *thread_function(void *arg)
{
	while (1)
	{
		int *pClient;
		pthread_mutex_lock(&queue_mutex);

		while (taskCount == 0)
		{
			pthread_cond_wait(&taskReady, &queue_mutex);
		}
		if (taskCount > 0)
		{
			found = 1;
			pClient = dequeue();
			taskCount--;
			pthread_cond_signal(&taskReady);
		}

		pthread_mutex_unlock(&queue_mutex);

		if (found == 1)
			grader(pClient);
	}
}

void masterFunc(int *pClient)
{
	pthread_mutex_lock(&queue_mutex);

	while ((rear + 1) % QUEUE_SIZE == front)
	{
		pthread_cond_wait(&taskReady, &queue_mutex);
	}

	enqueue(pClient);
	taskCount++;
	pthread_cond_signal(&taskReady);
	pthread_mutex_unlock(&queue_mutex);
}

void enqueue(int *client_sockfd)
{
	rear = (rear + 1) % QUEUE_SIZE;
	if (front == rear)
	{
		printf("\nOverflow\n");
		if (rear == 0)
			rear = QUEUE_SIZE - 1;
		else
			rear--;
	}
	else
	{
		queue[rear] = client_sockfd;
	}
}
int *dequeue()
{
	int *item;
	if (front == rear)
	{
		printf("\nThe Queue is empty\n");
	}
	else
	{
		front = (front + 1) % QUEUE_SIZE;
		item = queue[front];
		return item;
	}
}
