#include "stdio.h"
#include "string.h"

typedef struct request {
    int pid;
    char *input_filename;
    char *output_filename;
    char **args;
    int n_filters[5]; // array com o numero de cada filtro [0]  alto [1] baixo [2] ...
    struct request *prox;
} * REQUEST;

REQUEST init_request(int pid) {
    REQUEST new = malloc(sizeof(struct request));
    new->pid = pid;
    new->prox = NULL;
    return new;
}

int fill_request(REQUEST *req, char **args) { // args talvez seja um array de strings
}
