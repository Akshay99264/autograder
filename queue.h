#include <pthread.h>

typedef struct 
{
    char **values;
    int front,rear,size,numEntries;
}queue;

void init_queue(queue *q,int max_size);
void *enqueue(queue *q,char *client_sockfd);
char *dequeue(queue *q);
int q_size(queue *q);
int is_empty(queue *q);
int is_full(queue* q);
void *start_function(int *sockfd);
void error(char *msg);