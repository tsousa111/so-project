#include "request.h"
#include <stdio.h>
#include <string.h>

REQUEST create_new_request(char **args, int *n_filters) { // args talvez seja um array de strings
    REQUEST new = malloc(sizeof(struct request));
    new->input_filename = strdup(args[0]);
    new->output_filename = strdup(args[1]);
    new->args = args[2];
    int n_args = 0;
    for (int i = 0; i < 5; i++)
        n_args += n_filters[i];
    new->n_args = n_args;
    new->n_filters = n_filters;
    new->prox = NULL;
}

void enqueue(REQUEST *q, REQUEST entry) {
    if (!(*q))
        *q = entry;
    REQUEST aux;
    for (aux = *q; aux && aux->prox; aux = aux->prox)
        ;
    aux->prox = entry;
}

REQUEST dequeue(REQUEST *q) {
    REQUEST res = *q;
    q = &((*q)->prox);
    res->prox = NULL;
    return res;
}

void remove(REQUEST *q, int pid) {
    if (!(*q) && !((*q)->prox))
        free(*q);
    REQUEST aux;
    REQUEST removed;
    for (aux = *q; aux && aux->prox; aux = aux->prox) {
        if (aux->prox->pid == pid) {
            removed = aux->prox;
            aux->prox = removed->prox;
            free(removed);
            break;
        }
    }
}