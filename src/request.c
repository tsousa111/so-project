#include "request.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

REQUEST create_new_request(char **args, int *client_filters) { // args talvez seja um array de strings
    REQUEST new = malloc(sizeof(struct request));
    new->input_filename = strdup(args[0]);
    new->output_filename = strdup(args[1]);
    new->args = &(args[2]);
    int n_args = 0;
    for (int i = 0; i < 5; i++)
        n_args += client_filters[i];
    new->n_args = n_args;
    new->client_filters = client_filters;
    new->prox = NULL;
    return new;
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

void remove_request(REQUEST *q, int pid) {
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