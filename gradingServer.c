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
#include <uuid/uuid.h>
#include <libpq-fe.h>

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
pthread_mutex_t uuid_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t res_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t res_ready = PTHREAD_COND_INITIALIZER;

// evaluation threads
void *eval_thread_function(void *arg);
void eval_enqueue(char *request_id);
void *eval_masterFunc();
char *eval_dequeue();
int queue_position(char *request_id);
int eval_front = 0, eval_rear = 0, eval_count = 0, eval_found = 0, eval_taskCount = 0;
char *eval_queue[QUEUE_SIZE];
pthread_mutex_t eval_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t eval_task_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t eval_taskReady = PTHREAD_COND_INITIALIZER;

// Initialize a PostgreSQL connection object
PGconn *conn;
PGresult *res;

void error(char *msg)
{
	perror(msg);
	exit(1);
}

int recv_file(int sockfd, char *file_path)
{
	char buffer[BUFFER_SIZE];
	memset(buffer,0,sizeof(buffer));
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
		memset(buffer,0,sizeof(buffer));
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
	strcat(s, " 2> output/compiler_err_");
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
	strcat(s, " > output/out_");
	strcat(s, request_id);
	strcat(s, ".txt");
	strcat(s, " 2> output/runtime_err_");
	strcat(s, request_id);
	strcat(s, ".txt");
	return s;
}

char *output_diff_command(char *request_id)
{
	char *s;
	s = malloc(200 * sizeof(char));
	memset(s, 0, sizeof(s));
	strcpy(s, "diff output/out_");
	strcat(s, request_id);
	strcat(s, ".txt");
	strcat(s, " actualOutput.txt 1> output/diff_err_");
	strcat(s, request_id);
	strcat(s, ".txt");
	return s;
}

char *makeProgramFileName(char *request_id)
{
	char *s;
	s = malloc(200 * sizeof(char));
	memset(s, 0, sizeof(s));
	strcpy(s, "files/file_");
	strcat(s, request_id);
	strcat(s, ".c");
	return s;
}

char *makeCompileErrorFileName(char *request_id)
{
	char *s;
	s = malloc(200 * sizeof(char));
	memset(s, 0, sizeof(s));
	strcpy(s, "output/compiler_err_");
	strcat(s, request_id);
	strcat(s, ".txt");
	return s;
}

char *makeRuntimeErrorFileName(char *request_id)
{
	char *s;
	s = malloc(200 * sizeof(char));
	memset(s, 0, sizeof(s));
	strcpy(s, "output/runtime_err_");
	strcat(s, request_id);
	strcat(s, ".txt");
	return s;
}

char *makeOutputFileName(char *request_id)
{
	char *s;
	s = malloc(200 * sizeof(char));
	memset(s, 0, sizeof(s));
	strcpy(s, "files/out_");
	strcat(s, request_id);
	strcat(s, ".txt");
	return s;
}

char *makeExecFileName(char *request_id)
{

	char *s;
	s = malloc(200 * sizeof(char));
	memset(s, 0, sizeof(s));
	strcpy(s, "output/prog_");
	strcat(s, request_id);
	return s;
}

char *makeOutputDiffFileName(char *request_id)
{

	char *s;
	s = malloc(200 * sizeof(char));
	memset(s, 0, sizeof(s));
	strcpy(s, "output/diff_err_");
	strcat(s, request_id);
	strcat(s, ".txt");
	return s;
}

char *insertQuery(char *request_id, int status, char *error)
{

	char *s;
	char s1[5];
	
	s = malloc(2000 * sizeof(char));
	memset(s1, 0, sizeof(s1));
	sprintf(s1, "%d", status);
	memset(s, 0, sizeof(s));
	strcpy(s, "INSERT INTO grading_requests VALUES('");
	strcat(s, request_id); // request id
	strcat(s, "',");
	strcat(s, s1);         // status
	if(sizeof(error) > 0){
		strcat(s, ",'");
		strcat(s, error);      // error
		strcat(s, "'");
	}
	strcat(s, ")");
	return s;
}

char *updateQuery(char *request_id, int status, char *error)
{

	char *s;
	char s1[5];
	
	s = malloc(2000 * sizeof(char));
	memset(s1, 0, sizeof(s1));
	sprintf(s1, "%d", status);
	memset(s, 0, sizeof(s));
	strcpy(s, "UPDATE grading_requests SET status = ");
	// strcat(s, "',");
	strcat(s, s1);         // status
	if(sizeof(error) > 0){
		strcat(s, ",error = '");
		strcat(s, error);      // error
		strcat(s, "'");
	}
    strcat(s, " WHERE id ='");
	strcat(s, request_id); // request id
    strcat(s, "'");
	// strcat(s, ")");
	return s;
}

char *filterQuery()
{

	char *s;
	s = malloc(2000 * sizeof(char));
	memset(s, 0, sizeof(s));
	strcpy(s, "SELECT id FROM grading_requests WHERE status = 0");
	return s;
}

char *getQuery(char *request_id)
{

	char *s;
	
	s = malloc(2000 * sizeof(char));
	memset(s, 0, sizeof(s));
	strcpy(s, "SELECT status, error FROM grading_requests WHERE id = '");
    strcat(s, request_id);
    strcat(s, "'");
	return s;
}

char *statusUpdateQuery()
{

	char *s;
	
	s = malloc(2000 * sizeof(char));
	memset(s, 0, sizeof(s));
	strcpy(s, "UPDATE grading_requests SET status = 0 WHERE status = 1 OR status = 2");
	return s;
}

void *grader(int *sockfd)
{
	int newsockfd = *(int *)sockfd;
	int n;
	char request_type[10];
	char request_id[37];
	char *programFile, *execFile, *compileErrorFile, *runtimeErrorFile, *outputFile, *outputDiffFile;

	n = recv(newsockfd, request_type, sizeof(request_type), 0);
	if (n < 0)
		error("ERROR on accept request type");
	if (strcmp(request_type, "new") == 0 || strcmp(request_type, "New") == 0)
	{
		uuid_t uuid;
		pthread_mutex_lock(&uuid_mutex);
		uuid_generate(uuid);
		uuid_unparse(uuid, request_id);
		pthread_mutex_unlock(&uuid_mutex);
		
		programFile = makeProgramFileName(request_id);

		if (recv_file(newsockfd, programFile) != 0)
		{
			close(newsockfd);
			return 0;
		}
		
		// save in database
		char buff[2];
		strcpy(buff,"");
		char *query = insertQuery(request_id, 0, buff);
		PQclear(res);
		res = PQexec(conn, query);
		if(PQresultStatus(res) != PGRES_COMMAND_OK)
		{
			fprintf(stderr,"%s\n",PQerrorMessage(conn));
		}

        // signal to evalution thread
        pthread_cond_signal(&res_ready);

		char response[90];
		strcat(response,"I got your response and this is your request ID: ");
		strcat(response,request_id);
		n=send(newsockfd,response,sizeof(response)-2,0);
		if(n<0)
		{
			perror("Error sending response\n");
			close(newsockfd);
			return NULL;
		}
		close(newsockfd);
	}
	else
	{
		//code to handle check status case
        n = recv(newsockfd, request_id, 37, 0);
        char *query = getQuery(request_id);
        PQclear(res);
        res = PQexec(conn, query);
        int rows = PQntuples(res);
        if (rows == 0){
			char msg[150];
			snprintf(msg, 150, "Grading request %s not found. Please check and resend your request ID or re-send your original grading request.\0",request_id);
            n = send(newsockfd, msg, sizeof(msg), 0);
            if (n < 0)
                error("ERROR writing to socket");
        }
        else{
            int status = atoi(PQgetvalue(res, 0, 0));
            char *error_msg = PQgetvalue(res, 0, 1);
            
			printf("status: %d", status);

            if (status == 2){
                // not evaluated
				char msg[150];
				snprintf(msg,150,"Your grading request ID %s has been accepted and is currently being processed.\0",request_id);
                n = send(newsockfd, msg, sizeof(msg), 0);
                if (n < 0)
                    error("ERROR writing to socket");
            }
			else if (status == 0){
                // not evaluated
				char msg[150];
				snprintf(msg,150,"Your grading request ID %s has been accepted and will be processed soon.\0",request_id);
                n = send(newsockfd, msg, sizeof(msg), 0);
                if (n < 0)
                    error("ERROR writing to socket");
            }
            else if (status == 1){
                // in queue
                int position = queue_position(request_id);
                char s[150];
                snprintf(s, 150, "Your grading request ID %s has been accepted. It is currently at position %d in the queue.\n\0",request_id,position);
                n = send(newsockfd, s, sizeof(s), 0);
                if (n < 0)
                    error("ERROR writing to socket");
            }
            else if (status == 6){
                char msg[150];
				snprintf(msg, 150, "Your grading request ID %s processing is done, here are the results: PASS\n",request_id);
                n = send(newsockfd, msg, sizeof(msg), 0);
                if (n < 0)
                    error("ERROR writing to socket");
            }
            else {
				char error_output[1000];
				char msg[150];
				snprintf(msg, 150, "Your grading request ID %s processing is done, here are the results:\n",request_id);
				strcat(error_output,msg);
				strcat(error_output,error_msg);
				int len=strlen(error_output);
				error_output[len]='\0';
                n = send(newsockfd, error_output, len, 0);
                if (n < 0)
                    error("ERROR writing to socket");
            }
        }
	}
}

void evaluate_file(char *request_id)
{
    char *programFile, *execFile, *compileErrorFile, *runtimeErrorFile, *outputFile, *outputDiffFile;
    programFile = makeProgramFileName(request_id);
    execFile = makeExecFileName(request_id);
    compileErrorFile = makeCompileErrorFileName(request_id);
    runtimeErrorFile = makeRuntimeErrorFileName(request_id);
    outputFile = makeOutputFileName(request_id);
    outputDiffFile = makeOutputDiffFileName(request_id);
    char *comp_command, *r_command, *diff_command;
    comp_command = compile_command(request_id, programFile, execFile);
    r_command = run_command(request_id, execFile);
    diff_command = output_diff_command(request_id);

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

		// save to database
		char *query = updateQuery(request_id, 3, compilerErrorBuffer);
		PQclear(res);
		res = PQexec(conn, query);

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

		// save to database
		char *query = updateQuery(request_id, 4, runtimeErrorBuffer);
		PQclear(res);
		res = PQexec(conn, query);
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
		
		// save to database
		char *query = updateQuery(request_id, 5, diffErrorBuffer);
		PQclear(res);
		res = PQexec(conn, query);
		fclose(f);
	}
	else
	{
		// Send success message
		char buff[1];
		char *query = updateQuery(request_id, 6, buff);
		PQclear(res);
		res = PQexec(conn, query);
		// save to database
	}
    free(programFile);
    free(execFile);
    free(comp_command);
    free(r_command);
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

	// receive file thread pool
	pthread_t threadpool[MAX_THREADS];
	for (int i = 0; i < MAX_THREADS; i++)
	{
		pthread_create(&threadpool[i], NULL, thread_function, NULL);
	}

	// evaluation thread pool
	pthread_t eval_threadpool[MAX_THREADS];
	for (int i = 0; i < MAX_THREADS; i++)
	{
		pthread_create(&eval_threadpool[i], NULL, eval_thread_function, NULL);
	}

	// Postgres connection
	conn = PQconnectdb("dbname=autograder user=grader_user password=password host=localhost");
	// Check for a successful connection
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        return 1;
    }

    // const char *createTableSQL = "CREATE TABLE IF NOT EXISTS grading_requests (id uuid primary key, status int, error text null default null)";
    // res = PQexec(conn, createTableSQL);

    // if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    //     fprintf(stderr, "Table creation failed: %s", PQerrorMessage(conn));
    //     PQclear(res);
    //     PQfinish(conn);
    //     exit(1);
    // }

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

	pthread_t eval_thread;
	pthread_create(&eval_thread, NULL, eval_masterFunc, NULL);
	pthread_detach(eval_thread);

	// eval_masterFunc();
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

	pthread_mutex_destroy(&eval_queue_mutex);
	pthread_cond_destroy(&eval_taskReady);

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

// evaluation threads function
void *eval_thread_function(void *arg)
{
	while(1){
		char *pClient; 
		pthread_mutex_lock(&eval_queue_mutex);
		while (eval_taskCount == 0)
		{
			pthread_cond_wait(&eval_taskReady, &eval_queue_mutex);
		}
		if (eval_taskCount > 0)
		{
			eval_found = 1;
			pClient = eval_dequeue();
			eval_taskCount--;
			char *query = updateQuery(pClient, 2, "");
			PGresult *update_res = PQexec(conn, query);
			if(PQresultStatus(update_res) != PGRES_COMMAND_OK)
			{
				fprintf(stderr,"%s\n",PQerrorMessage(conn));
			}

			pthread_cond_signal(&eval_taskReady);
		}

		pthread_mutex_unlock(&eval_queue_mutex);
		if (eval_found == 1)
			evaluate_file(pClient);
	}
	
}

void *eval_masterFunc()
{
	sleep(2);
    char *query = statusUpdateQuery();
    PQclear(res);
    res = PQexec(conn, query);
    if(PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        fprintf(stderr,"%s\n",PQerrorMessage(conn));
    }
	while (1)
	{
        query = filterQuery();
        PQclear(res);
		res = PQexec(conn, query);
		if(PQresultStatus(res) != PGRES_COMMAND_OK)
		{
			fprintf(stderr,"%s\n",PQerrorMessage(conn));
		}
        int rows = PQntuples(res);
        pthread_mutex_lock(&res_mutex);
        while (rows == 0)
        {
            pthread_cond_wait(&res_ready, &res_mutex);
			PQclear(res);
			res = PQexec(conn, query);
			if(PQresultStatus(res) != PGRES_COMMAND_OK)
			{
				fprintf(stderr,"%s\n",PQerrorMessage(conn));
			}
			rows = PQntuples(res);
        }
        pthread_mutex_unlock(&res_mutex);

        for(int i=0; i<rows; i++) {
            char *request_id = PQgetvalue(res, i, 0);
			pthread_mutex_lock(&eval_queue_mutex);
			while ((eval_rear + 1) % QUEUE_SIZE == eval_front)
			{
				pthread_cond_wait(&eval_taskReady, &eval_queue_mutex);
			}

			// get uuid from database
			eval_enqueue(request_id);

			char *query = updateQuery(request_id, 1, "");
			PGresult *update_res = PQexec(conn, query);
			if(PQresultStatus(update_res) != PGRES_COMMAND_OK)
			{
				fprintf(stderr,"%s\n",PQerrorMessage(conn));
			}

			eval_taskCount++;
			pthread_cond_signal(&eval_taskReady);
			pthread_mutex_unlock(&eval_queue_mutex);
		}
	}
}

void eval_enqueue(char *request_id)
{
	eval_rear = (eval_rear + 1) % QUEUE_SIZE;
	if (eval_front == eval_rear)
	{
		printf("\nOverflow\n");
		if (eval_rear == 0)
			eval_rear = QUEUE_SIZE - 1;
		else
			eval_rear--;
	}
	else
	{
		eval_queue[eval_rear] = request_id;
	}
}
char *eval_dequeue()
{
	char *item;
	if (eval_front == eval_rear)
	{
		printf("\nThe Queue is empty\n");
	}
	else
	{
		eval_front = (eval_front + 1) % QUEUE_SIZE;
		item = eval_queue[eval_front];
		return item;
	}
}

int queue_position(char *request_id)
{
    int i;
    for (i = 0; i < QUEUE_SIZE; i++){
        if (eval_queue[i] == request_id){
            break;
        }
    }

    if (i > eval_front)
    {
        return eval_front - i;
    }
    else
    {
        return QUEUE_SIZE - front + i;
    }
}