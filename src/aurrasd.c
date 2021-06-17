#include "request.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define CLIENT_REQ_ID 64
#define MAX_BUF 1024
#define main_fifo "../tmp/main_req_fifo"
#define finished_req_fifo "../tmp/finished_req_fifo"

char *cfg_array[30] = {0};
char filter_path[64];
int n_filters = 0;
int *max_filters;
int *filters_being_used;
REQUEST request_queue = NULL;
REQUEST request_executing = NULL;

int closing_flag = 0;

void sighandler_sigusr1(int signum) {
    int pid;
    int flag = 0;
    int fd_finished_req = open(finished_req_fifo, O_RDONLY);
    read(fd_finished_req, &pid, sizeof(int));
    close(fd_finished_req);
    printf("PID_FINISHED : %d\n", pid);
    REQUEST removed = remove_request(&request_executing, pid);

    for (int i = 0; i < n_filters; i++) {
        filters_being_used[i] -= removed->client_filters[i];
    }

    while (request_queue != NULL && !flag) {
        for (int i = 0; i < n_filters && !flag; i++) {
            if ((request_queue->client_filters[i] + filters_being_used[i]) > max_filters[i])
                flag = 1;
        }
        if (!flag) {
            REQUEST aux = dequeue(&request_queue);
            enqueue(&request_executing, aux);
            for (int i = 0; i < n_filters; i++) // atualizar os filtros que estao a ser usados
                filters_being_used[i] += aux->client_filters[i];
            printf("Sent signal to %d\n", aux->pid);
            kill(aux->pid, SIGUSR2);
        }
    }
}

void sighandler_sigusr2(int signum) {}

void sighandler_sigint(int signum) {
    int fd = open(main_fifo, O_WRONLY);
    write(fd, "close", strlen("close"));
    close(fd);
}

void status_message(char *res) {
    char aux[128];
    REQUEST aux_req = request_executing;
    strcat(res, "Executing :\n");
    while (aux_req != NULL) {
        sprintf(aux, "\t%s\n", aux_req->original_request);
        strcat(res, aux);

        aux_req = aux_req->prox;
    }
    strcat(res, "\n");
    aux_req = request_queue;
    strcat(res, "In queue :\n");
    while (aux_req != NULL) {
        sprintf(aux, "\t%s\n", aux_req->original_request);
        strcat(res, aux);
        aux_req = aux_req->prox;
    }
    strcat(res, "\n");

    for (int i = 0; i < n_filters; i++) {
        sprintf(aux, "filter %s: %d/%d (running/max)\n", cfg_array[i * 3], filters_being_used[i], max_filters[i]);
        strcat(res, aux);
    }
}

int parse_str_to_str_array(char *string, char **str_array) {
    int n_words = 0;
    char delimit[] = " \n";
    char *token = NULL;
    token = strtok(string, delimit);
    while (token != NULL) {
        str_array[n_words] = token;
        n_words++;
        token = strtok(NULL, delimit);
    }
    return n_words;
}

int parse_request(char **parsed_request, int *client_filters, int n_args) {
    int flag = 0;
    for (int i = 3; i < n_args && flag != -1; i++) {
        for (int j = 0; cfg_array[j]; j = j + 3) {
            if (strcmp(parsed_request[i], cfg_array[j]) == 0) {
                parsed_request[i] = cfg_array[j + 1];
                client_filters[j / 3]++;
                flag = 1;
                break;
            }
        }
        if (!flag)
            flag = -1;
    }
    return flag;
}

int main(int argc, char const *argv[]) {
    int pid;
    char config_buffer[MAX_BUF];
    // request id dado pelo cliente
    // falta o load da config
    if (argc != 3) {
        printf("Incorrect server inputs...\n");
        return 1;
    } else {
        // leitura e parsing da config
        strcat(filter_path, argv[2]);
        strcat(filter_path, "/");

        int fd_config;
        if ((fd_config = open(argv[1], O_RDONLY)) == -1) {
            printf("Error opening config file...\n");
            return 1;
        }
        read(fd_config, config_buffer, sizeof(config_buffer));
        close(fd_config);
        int n_words = parse_str_to_str_array(config_buffer, cfg_array);

        n_filters = n_words / 3;

        max_filters = calloc(n_filters, sizeof(int));
        filters_being_used = calloc(n_filters, sizeof(int));
        for (int i = 2; i < n_words; i += 3) {
            max_filters[(i - 2) / 3] = atoi(cfg_array[i]);
        }
    }

    signal(SIGUSR1, sighandler_sigusr1);
    signal(SIGUSR2, sighandler_sigusr2);
    signal(SIGINT, sighandler_sigint);

    mkfifo(main_fifo, 0666);

    mkfifo(finished_req_fifo, 0666);

    printf("Server ON.\n");

    int main_req_fifo;

    while (1) {
        int *client_filters = calloc(n_filters, sizeof(int)); // numero de filtros usados pelo cliente
        char cl_sv_fifo[64] = "../tmp/cl_sv_";
        char sv_cl_fifo[64] = "../tmp/sv_cl_";
        char request[MAX_BUF] = {0};
        char cl_req_id[CLIENT_REQ_ID] = {0};
        int fd_cl_sv;

        main_req_fifo = open(main_fifo, O_RDONLY);

        read(main_req_fifo, cl_req_id, sizeof(cl_req_id));
        close(main_req_fifo);

        if (!strcmp(cl_req_id, "close"))
            break;

        strcat(cl_sv_fifo, cl_req_id);

        if (mkfifo(cl_sv_fifo, 0666) == -1) { // fifo com o pid do cliente para enviar as infos
            printf("Couldn't create cl_sv_fifo\n");
            return 1;
        }

        if ((fd_cl_sv = open(cl_sv_fifo, O_RDONLY)) == -1) {
            printf("Couldn't open fd_cl_sv\n");
            return 1;
        }

        read(fd_cl_sv, request, sizeof(request)); // read request dado pelo cliente

        printf("%s : %s\n", cl_req_id, request);

        char *parsed_request[20];
        int n_args;
        REQUEST req = NULL;
        int filter_flag = 0;
        char *request_copy = strdup(request);
        n_args = parse_str_to_str_array(request, parsed_request);
        int invalid_filter = 0;
        // se o nome de um filter nao for valido devolve -1
        if ((invalid_filter = parse_request(parsed_request, client_filters, n_args)) != -1) {
            if (!strcmp(parsed_request[0], "transform")) { // se for transform cria um request

                if (n_args > 3) {
                    req = create_new_request(&(parsed_request[1]), client_filters, n_filters, request_copy);

                    for (int i = 0; i < n_filters && !filter_flag; i++) {
                        if (req->client_filters[i] > max_filters[i]) // se a quantidade de um filtro exceder o seu maximo
                            filter_flag = -1;

                        else if ((req->client_filters[i] + filters_being_used[i]) > max_filters[i]) // se nao houver filtros suficientes
                            filter_flag = 1;                                                        // para satisfazer o pedido do cliente
                    }
                    if (request_queue != NULL) {
                        filter_flag = 1;
                    }
                    for (int i = 0; i < n_filters && !filter_flag; i++) { // atualizar os filtros que estao a ser usados
                        filters_being_used[i] += req->client_filters[i];
                    }
                } else
                    n_args = -1;
            }
        }
        if ((pid = fork()) == 0) {
            int fd_sv_cl;
            strcat(sv_cl_fifo, cl_req_id);
            char *message;

            if (mkfifo(sv_cl_fifo, 0666) == -1) { // fifo para sv mandar ao cliente info
                printf("Couldn't create client_req_id_fifo\n");
                _exit(-1);
            }
            if ((fd_sv_cl = open(sv_cl_fifo, O_WRONLY)) == -1) {
                printf("Couldn't open fd_cl_sv");
                _exit(-1);
            }

            if (invalid_filter == -1 || n_args == -1) {
                message = strdup("Invalid filter name!\n");
                write(fd_sv_cl, message, strlen(message));
            } else if (filter_flag == -1) {
                message = strdup("Filter limit reached!\n");
                write(fd_sv_cl, message, strlen(message));
            } else if (req != NULL) { // comando transform
                // message = strdup("Pending...");
                // write(fd_sv_cl, message, strlen(message));
                if (filter_flag) // fica a espera quando nao tem filtros disponiveis
                    pause();

                // message = strdup("Processing...");
                // write(fd_sv_cl, message, strlen(message));

                if (req->n_args == 1) {
                    if (fork() == 0) {
                        strcat(filter_path, req->args[0]);

                        int input_fd = open(req->input_filename, O_RDONLY);
                        int output_fd = open(req->output_filename, O_CREAT | O_WRONLY, 0666);

                        dup2(input_fd, 0);
                        close(input_fd);

                        dup2(output_fd, 1);
                        close(output_fd);

                        execl(filter_path, filter_path, NULL);
                        _exit(-1);
                    } else {
                        int status;
                        wait(&status);
                    }
                } else {
                    int status;
                    int pd[req->n_args - 1][2];
                    pipe(pd[0]);
                    for (int i = 0; i < req->n_args; i++) {
                        if (i == 0) {
                            if (fork() == 0) {
                                strcat(filter_path, req->args[i]);
                                close(pd[0][0]);

                                int input_fd = open(req->input_filename, O_RDONLY);

                                dup2(input_fd, 0);
                                close(input_fd);

                                dup2(pd[0][1], 1);
                                close(pd[0][1]);

                                execl(filter_path, filter_path, NULL);
                            } else {
                                close(pd[0][1]);
                            }

                        } else if (i == req->n_args - 1) {
                            if (fork() == 0) {
                                strcat(filter_path, req->args[i]);

                                int output_fd = open(req->output_filename, O_CREAT | O_WRONLY, 0666);
                                dup2(output_fd, 1);

                                dup2(pd[i - 1][0], 0);
                                close(pd[i - 1][0]);

                                execl(filter_path, filter_path, NULL);
                                _exit(-1);
                            } else {
                                close(pd[i - 1][0]);
                            }
                        } else {
                            pipe(pd[i]);
                            while (request_executing != NULL || request_queue != NULL) {
                                pause();
                            }
                            if (fork() == 0) {
                                strcat(filter_path, req->args[i]);
                                close(pd[i][0]);

                                dup2(pd[i - 1][0], 0);
                                close(pd[i - 1][0]);

                                dup2(pd[i][1], 1);
                                close(pd[i][1]);

                                execl(filter_path, filter_path, NULL);
                                _exit(-1);
                            } else {
                                close(pd[i][1]);
                                close(pd[i - 1][0]);
                            }
                        }
                    }
                    for (int i = 0; i < req->n_args; i++) {
                        wait(&status);
                    }

                    kill(getppid(), SIGUSR1);

                    int fd_finished_req = open(finished_req_fifo, O_WRONLY);
                    int pid_finished = getpid();

                    write(fd_finished_req, &pid_finished, sizeof(int));
                    close(fd_finished_req);
                }
            } else if (!strcmp(parsed_request[0], "status")) {
                char status_write[2048] = {0};
                status_message(status_write);
                printf("%s\n", status_write);
                write(fd_sv_cl, status_write, strlen(status_write));
            } else {
                message = strdup("Invalid arguments!\n");
                write(fd_sv_cl, message, strlen(message));
            }
            write(fd_sv_cl, "Done!", 5);

            close(fd_sv_cl);
            unlink(sv_cl_fifo);
            _exit(0);
        } else {
            if (req != NULL) {
                req->pid = pid;
                if (filter_flag) {
                    enqueue(&request_queue, req);
                    printf("Added %d to the queue\n", request_queue->pid);
                } else if (filter_flag == 0) {
                    enqueue(&request_executing, req);
                    printf("%d executing\n", request_executing->pid);
                }
            }
        }
        close(fd_cl_sv);
        unlink(cl_sv_fifo);
    }

    unlink(main_fifo);
    unlink(finished_req_fifo);
    return 0;
}