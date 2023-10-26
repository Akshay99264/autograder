// author : Ravi Patidar 23M0796

typedef struct 
{
    int **values;
    int front,rear,size,numEntries;
}queue;

void init_queue(queue *q,int max_size);
int *enqueue(queue *q,int *client_sockfd);
int *dequeue(queue *q);
int q_size(queue *q);
int is_empty(queue *q);
int is_full(queue* q);
void *start_function(int *sockfd);
void error(char *msg);