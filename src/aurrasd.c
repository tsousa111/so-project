#include "request.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define CLIENT_REQ_ID 64
#define MAX_BUF 1024
#define main_fifo "../tmp/cl_sv_fifo"

char *cfg_array[30] = {0};
char filter_path[64];
int n_filters;
int max_filters[5] = {0};
int filters_being_used[5] = {0};
int signal_pid_pd[2];

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
        for (int j = 0, k = 0; cfg_array[j]; j = j + 3, k++) {
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
        strcpy(filter_path, argv[2]);
        strcat(filter_path, "/");

        int fd_config;
        if ((fd_config = open(argv[1], O_RDONLY)) == -1) {
            printf("Error opening config file...\n");
            return 1;
        }
        read(fd_config, config_buffer, MAX_BUF);
        close(fd_config);
        int n_words = parse_str_to_str_array(config_buffer, cfg_array);
        n_filters = n_words / 3;
    }

    mkfifo(main_fifo, 0666);
    // mkfifo(Client_Server_Main, 0666);
    printf("Server ON.\n");

    //Abrir o main pipe.
    // int fd_client_server_main;
    // fd_client_server_main = open(Client_Server_Main, O_RDONLY);

    // printf("Sv On\n");

    int fd_cl_server;
    fd_cl_server = open(main_fifo, O_RDONLY);

    printf("OHOH FOK YOU\n");
    while (1) {
        int client_filters[10] = {0}; // numero de filtros usados pelo cliente
        char cl_sv_fifo[64] = "../tmp/cl_sv_";
        char sv_cl_fifo[64] = "../tmp/sv_cl_";
        char request[MAX_BUF] = {0};
        int fd_cl_sv;
        char cl_req_id[CLIENT_REQ_ID] = {0};

        int bytes = read(fd_cl_server, cl_req_id, CLIENT_REQ_ID);
        printf("%d\n", bytes);

        strcat(cl_sv_fifo, cl_req_id);
        if (mkfifo(cl_sv_fifo, 0666) == -1) { // fifo com o pid do cliente para enviar as infos
            printf("Couldn't create cl_sv_fifo\n");
            _exit(-1);
        }

        if ((fd_cl_sv = open(cl_sv_fifo, O_RDONLY)) == -1) {
            printf("Couldn't open fd_cl_sv\n");
            _exit(-1);
        }

        read(fd_cl_sv, request, MAX_BUF); // read request dado pelo cliente

        char *parsed_request[20];
        int n_args;
        REQUEST req = NULL;
        int filter_flag = 0;

        n_args = parse_str_to_str_array(request, parsed_request);

        // se o nome de um filter nao for valido devolve -1
        if (parse_request(parsed_request, client_filters, n_args) != -1) {
            if (!strcmp(parsed_request[0], "transform")) { // se for transform cria um request
                if (n_args > 3) {
                    req = create_new_request(&(parsed_request[1]), client_filters);

                    for (int i = 0; i < n_filters && !filter_flag; i++) {
                        if (req->client_filters[i] > max_filters[i]) // se a quantidade de um filtro exceder o seu maximo
                            filter_flag = -1;

                        else if ((req->client_filters[i] + filters_being_used[i]) > max_filters[i]) { // se nao houver filtros suficientes
                            filter_flag = 1;                                                          // para satisfazer o pedido do cliente
                        }
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
            if ((fd_sv_cl = open(sv_cl_fifo, O_WRONLY, 0666)) == -1) {
                printf("Couldn't open fd_cl_sv");
                _exit(-1);
            }
            if (n_args == -1) {
                message = strdup("Invalid filter name!\n");
                write(fd_sv_cl, message, strlen(message));
            } else if (filter_flag == -1) {
                message = strdup("Filter limit reached!\n");
                write(fd_sv_cl, message, strlen(message));
            } else if (req != NULL) { // comando transform
                // write pending to fifo
                // if (filter_flag) // fica a espera quando nao tem filtros disponiveis
                //     pause();
                // write processing to fifo

                // if (req->n_args == 1) {
                // int input_fd = open(req->input_filename, O_RDONLY);
                // int output_fd = open(req->output_filename, O_CREAT | O_RDONLY, 0666);
                //
                // dup2(input_fd, 0);
                // dup2(output_fd, 1);
                // close(input_fd);
                // close(output_fd);
                // execl(req->args[0], req->args[0], NULL);
                // } else {
                // int pd[n_args - 1][2];
                // }
                printf("Received %s \n", req->args[0]);
            } else if (!strcmp(parsed_request[0], "status")) {
                if (n_args > 1) {
                    message = strdup("Too many arguments for status call!\n");
                    write(fd_sv_cl, message, strlen(message));
                } else {
                    // send to client the current status
                }
            } else {
                message = strdup("Invalid arguments!\n");
                write(fd_sv_cl, message, strlen(message));
            }
            close(fd_sv_cl);
            unlink(sv_cl_fifo);

        } else {
            if (req != NULL) {
                req->pid = pid;
            }
            // add to queue;
        }
        close(fd_cl_sv);
        unlink(cl_sv_fifo);
    }
    close(fd_cl_server);
    return 0;
}