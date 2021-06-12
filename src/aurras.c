#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_BUF 1024

int main(int argc, char const *argv[]) {
    int fd_req_fifo;
    char request[1024];
    char req_id[64];

    if ((fd_req_fifo = open("tmp/cl_sv_fifo", O_WRONLY) == -1)) {
        printf("Server closed... Try again later!\n");
        return -1;
    }

    if (argc == 2 && !strcmp(argv[1], "status")) {
        strcpy(argv[1], request);
    } else if (argc > 2 && !strcmp(argv[1], "transform")) { // TALVEZ MELHORAR
        strcpy(request, argv[1]);
        for (int i = 2; i < argc; i++) {
            strcat(request, " ");
            strcat(request, argv[i]);
        }
    } else {
        close(fd_req_fifo);
        printf("Bad input...");
        return -1;
    }

    sprintf(req_id, "%d", getpid());
    write(fd_req_fifo, req_id, strlen(req_id));
    close(fd_req_fifo);

    int fd_cl_sv, fd_sv_cl;
    char cl_sv_pid[25] = "../tmp/cl_sv_";
    char sv_cl_pid[25] = "../tmp/sv_cl_";

    strcat(cl_sv_pid, req_id);
    strcat(sv_cl_pid, req_id);
    sleep(1);
    if ((fd_cl_sv = open(cl_sv_pid, O_WRONLY) == -1)) {
        printf("Erro a abrir a fifo fd_cl_sv");
        return -1;
    }
    if ((fd_sv_cl = open(sv_cl_pid, O_RDONLY) == -1)) {
        printf("Erro a abrir a fifo fd_sv_cl");
        return -1;
    }

    write(fd_cl_sv, request, strlen(request));

    return 0;
}