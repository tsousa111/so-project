#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define CLIENT_REQ_ID 64
#define MAX_BUF 1024

char *filter_names[5];
char *filter_path;
int max_filters[5];

readln(int fd, char *line, size_t size) {
    ssize_t res = 0;
    int j = 0;
    char local[MAX_BUF];

    while ((res = read(fd, local, size)) > 0) {
        for (int i = 0; i < res; i++) {
            if (((char *)local)[i] != '\n') {
                line[j] = local[i];
                j++;
            } else {
                line[j++] = '\n';
                return j;
            }
        }
    }
    return j;
}

int parse(char *request, char **parsed_request) { // poderia ter feito de forma generica;
    char *token;
    token = strtok(request, " ");
    int i = 0;
    while (token != NULL) {
        if (i > 2) {
            int j;
            if (!strcmp(token, "alto"))
                j = 0;
            else if (!strcmp(token, "baixo"))
                j = 1;
            else if (!strcmp(token, "eco"))
                j = 2;
            else if (!strcmp(token, "rapido"))
                j = 3;
            else if (!strcmp(token, "lento"))
                j = 4;
            else
                return -1;
            parsed_request[i] = strdup(strcat(filter_path, filter_names[j]));
        } else {
            parsed_request[i] = strdup(token);
        }
        token = strtok(NULL, " ");
        i++;
    }
    return i;
}

int main(int argc, char const *argv[]) {
    int fd_cl_server;
    pid_t pid;
    char config_buffer[MAX_BUF];
    char cl_sv_pid[25] = "../tmp/cl_sv_";
    char sv_cl_pid[25] = "../tmp/sv_cl_";
    char cl_req_id[CLIENT_REQ_ID]; // request id dado pelo cliente

    // falta o load da config
    if (argc != 2) {
        printf("Incorrect server inputs...");
        return 1;
    } else {
        filter_path = strdup(argv[2]);
        int fd_config;
        if ((fd_config = open(argv[1], O_RDONLY)) == -1) {
            printf("Error opening config file...");
            return 1;
        }
        while (readln(fd_config, config_buffer, config_buffer) > 0) {
            char *token = strtok(config_buffer, " ");
            int i;
            if (!strcmp(token, "alto")) {
                i = 0;
            } else if (!strcmp(token, "baixo")) {
                i = 1;
            } else if (!strcmp(token, "eco")) {
                i = 2;
            } else if (!strcmp(token, "rapido")) {
                i = 3;
            } else if (!strcmp(token, "lento")) {
                i = 4;
            }
            filter_names[i] = strdup(strtok(NULL, " "));
            max_filters[i] = atoi(strtok(NULL, " "));
        }
        close(fd_config);
    }

    if (mkfifo("../tmp/cl_sv_fifo", 0666) == -1) {
        printf("Couldn't create client_server_fifo\n");
        return -1;
    }

    while (1) {
        if ((fd_cl_server = open("../tmp/cl_sv_fifo", O_RDONLY)) == -1) {
            printf("Could't open client_server_fifo");
            return -1;
        }
        int bytes_read = read(fd_cl_server, cl_req_id, CLIENT_REQ_ID);

        if ((pid = fork()) == 0) {
            int fd_sv_cl, fd_cl_sv;
            char request[MAX_BUF];
            char *cl_sv_fifo = strcat(cl_sv_pid, cl_req_id);
            char *sv_cl_fifo = strcat(sv_cl_pid, cl_req_id);
            char *message;

            if (mkfifo(cl_sv_fifo, 0666) == -1) { // fifo com o pid do cliente para enviar as infos
                printf("Couldn't create client_req_id_fifo\n");
                _exit(-1);
            }
            if (mkfifo(sv_cl_fifo, 0666) == -1) { // fifo para sv mandar ao cliente info
                printf("Couldn't create client_req_id_fifo\n");
                _exit(-1);
            }
            if (fd_cl_sv = open(cl_sv_fifo, O_RDONLY) == -1) {
                printf("Could't open fd_cl_sv");
                _exit(-1);
            }
            if (fd_sv_cl = open(sv_cl_fifo, O_WRONLY) == -1) {
                printf("Could't open fd_cl_sv");
                _exit(-1);
            }

            read(fd_cl_sv, request, MAX_BUF);
            char *parsed_request[20];
            int n_args = parse(request, parsed_request);
            if (strcmp(parsed_request[0], "transform")) {

            } else if (strcmp(parsed_request[0], "status")) {
                if (n_args > 1) {
                    message = strdup("Too many arguments for status call!\n");
                    write(fd_sv_cl, message, strlen(message));
                } else {
                    // send to client the current status
                }
            } else {
                message = strdup("Too many arguments for status call!\n");
                write(fd_sv_cl, message, strlen(message));
            }

            close(fd_cl_sv);
            close(fd_sv_cl);
            unlink(cl_sv_fifo);
            unlink(sv_cl_fifo);
        } else {
        }
        close(fd_cl_server);
    }

    return 0;
}