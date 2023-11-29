#include <stdio.h>

const int BUFFER_SIZE = 1024;
const int MAX_FILE_SIZE_BYTES = 4;

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

    while (1)
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

char *compile_command(int id, char *programFile, char *execFile)
{
    char *s;
    char s1[20];
    s = malloc(200 * sizeof(char));
    memset(s, 0, sizeof(s));
    memset(s1, 0, sizeof(s1));
    strcpy(s, "gcc -o ");
    strcat(s, execFile);
    strcat(s, "  ");
    strcat(s, programFile);
    strcat(s, " 2> files/compiler_err");
    sprintf(s1, "%d", id);
    strcat(s, s1);
    strcat(s, ".txt");
    return s;
}

char *run_command(int id, char *execFileName)
{
    char *s;
    char s1[20];
    s = malloc(200 * sizeof(char));
    memset(s, 0, sizeof(s));
    memset(s1, 0, sizeof(s1));
    sprintf(s1, "%d", id);
    strcpy(s, "./");
    strcat(s, execFileName);
    strcat(s, " > files/out");
    strcat(s, s1);
    strcat(s, ".txt");
    strcat(s, " 2> files/runtime_err");
    strcat(s, s1);
    strcat(s, ".txt");
    return s;
}

char *output_diff_command(int id)
{
    char *s;
    char s1[20];
    s = malloc(200 * sizeof(char));
    memset(s, 0, sizeof(s));
    memset(s1, 0, sizeof(s1));
    sprintf(s1, "%d", id);
    strcpy(s, "diff files/out");
    strcat(s, s1);
    strcat(s, ".txt");
    strcat(s, " actualOutput.txt 1> files/diff_err");
    strcat(s, s1);
    strcat(s, ".txt");
    return s;
}

char *makeProgramFileName(int id)
{
    char *s;
    char s1[20];
    s = malloc(200 * sizeof(char));
    memset(s, 0, sizeof(s));
    memset(s1, 0, sizeof(s1));
    sprintf(s1, "%d", id);
    strcpy(s, "files/file");
    strcat(s, s1);
    strcat(s, ".c");
    return s;
}

char *makeCompileErrorFileName(int id)
{
    char *s;
    char s1[20];
    s = malloc(200 * sizeof(char));
    memset(s, 0, sizeof(s));
    memset(s1, 0, sizeof(s1));
    sprintf(s1, "%d", id);
    strcpy(s, "files/compiler_err");
    strcat(s, s1);
    strcat(s, ".txt");
    return s;
}

char *makeRuntimeErrorFileName(int id)
{
    char *s;
    char s1[20];
    s = malloc(200 * sizeof(char));
    memset(s, 0, sizeof(s));
    memset(s1, 0, sizeof(s1));
    sprintf(s1, "%d", id);
    strcpy(s, "files/runtime_err");
    strcat(s, s1);
    strcat(s, ".txt");
    return s;
}

char *makeOutputFileName(int id)
{
    char *s;
    char s1[20];
    s = malloc(200 * sizeof(char));
    memset(s, 0, sizeof(s));
    memset(s1, 0, sizeof(s1));
    sprintf(s1, "%d", id);
    strcpy(s, "files/out");
    strcat(s, s1);
    strcat(s, ".txt");
    return s;
}

char *makeExecFileName(int id)
{

    char *s;
    char s1[20];
    s = malloc(200 * sizeof(char));
    memset(s, 0, sizeof(s));
    memset(s1, 0, sizeof(s1));
    sprintf(s1, "%d", id);
    strcpy(s, "files/prog");
    strcat(s, s1);
    return s;
}

char *makeOutputDiffFileName(int id)
{

    char *s;
    char s1[20];
    s = malloc(200 * sizeof(char));
    memset(s, 0, sizeof(s));
    memset(s1, 0, sizeof(s1));
    sprintf(s1, "%d", id);
    strcpy(s, "files/diff_err");
    strcat(s, s1);
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
