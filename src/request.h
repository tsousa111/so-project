#ifndef ___REQUEST___
#define ___REQUEST___

typedef struct request {
    int pid;
    char *input_filename;
    char *output_filename;
    char **args;
    int n_args;
    int *client_filters; // array com o numero de cada filtro [0]  alto [1] baixo [2] ...
    struct request *prox;
} * REQUEST;

REQUEST create_new_request(char **args, int *client_filters, int n_filters);

void enqueue(REQUEST *q, REQUEST entry);

REQUEST dequeue(REQUEST *q);

REQUEST remove_request(REQUEST *q, int pid);

void destroy_request(REQUEST r);

#endif