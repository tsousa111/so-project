#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define CLIENT_REQ_ID 64
#define MAX_BUF 1024

char **parse(char *request, char **parsed_request) {
    char *token;
    token = strtok(request, " ");
    int i = 0;
    while (token != NULL) {
        parsed_request[i] = strdup(token);
        token = strtok(NULL, " ");
        i++;
    }
    return i;
}

int main(int argc, char const *argv[]) {
    int fd_cl_server;
    pid_t pid;
    char cl_sv_pid[25] = "../tmp/cl_sv_";
    char sv_cl_pid[25] = "../tmp/sv_cl_";
    char cl_req_id[CLIENT_REQ_ID]; // request id dado pelo cliente

    // falta o load da config

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