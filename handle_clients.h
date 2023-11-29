queue client_queue, evalution_queue;

void clientsMasterFunc(int *pClient);
void *thread_function(void *arg);
void *grader(char *sockfd);
void error(char *msg);

