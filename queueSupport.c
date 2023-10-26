// author : Ravi Patidar 23M0796
#include<stdlib.h>
#include"support.h"

void init_queue(queue *q,int max_size)
{
    q->size=max_size;
    q->values=malloc(sizeof(int*)*q->size);
    q->front=0;
    q->rear=0;
    q->numEntries=0;
}
int *enqueue(queue *q,int *client_sockfd)
{
    q->rear = (q->rear + 1) % q->size;
    if (q->front == q->rear)
    {
        if (q->rear = 0)
            q->rear = q->size - 1;
        else
            q->rear=q->rear-1;
        return 0;
    }
    else
    {
        q->values[q->rear] = client_sockfd;
        q->numEntries=q->numEntries+1;
    }
}

int *dequeue(queue *q)
{
    int *item;
    if (q->front == q->rear)
    {
        return 0;
    }
    else
    {
        q->front = (q->front + 1) % q->size;
        item = q->values[q->front];
        q->numEntries=q->numEntries-1;
        return item;
    }
}
int q_size(queue *q) {
   return q->numEntries;
}

int is_empty(queue *q)
{
    if(q->numEntries==0) return 1;
    else return 0;
}
int is_full(queue* q)
{
    if(q->numEntries==q->size) return 1;
    else return 0;
}