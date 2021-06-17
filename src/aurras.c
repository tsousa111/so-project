#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define MAX_BUF 1024

int main(int argc, char const *argv[]) {
    int fd_req_fifo;
    char request[MAX_BUF];
    char req_id[64] = {0};

    if ((fd_req_fifo = open("../tmp/main_req_fifo", O_WRONLY)) == -1) {
        printf("Server closed... Try again later!\n");
        return -1;
    }
    if (argc == 2 && !strcmp(argv[1], "status")) {
        strcpy(request, argv[1]);
    } else if (argc > 2 && !strcmp(argv[1], "transform")) { // TALVEZ MELHORAR
        strcpy(request, argv[1]);
        for (int i = 2; i < argc; i++) {
            strcat(request, " ");
            strcat(request, argv[i]);
        }
    } else {
        close(fd_req_fifo);
        printf("Bad input...\n");
        return -1;
    }

    sprintf(req_id, "%d", (int)getpid());

    write(fd_req_fifo, req_id, sizeof(req_id));

    close(fd_req_fifo);

    int fd_cl_sv, fd_sv_cl;
    char cl_sv_pid[64] = "../tmp/cl_sv_";
    char sv_cl_pid[64] = "../tmp/sv_cl_";

    strcat(cl_sv_pid, req_id);
    strcat(sv_cl_pid, req_id);

    sleep(2);
    if ((fd_cl_sv = open(cl_sv_pid, O_WRONLY)) == -1) {
        printf("Erro a abrir a fifo fd_cl_sv\n");
        return -1;
    }

    write(fd_cl_sv, request, sizeof(request));

    close(fd_cl_sv);

    sleep(1);

    fd_sv_cl = open(sv_cl_pid, O_RDONLY);

    char message[2048] = {0};
    // while (strcmp(message, "Done!") != 0) {
    read(fd_sv_cl, message, sizeof(message));
    printf("%s\n", message);
    // memset(message, 0, sizeof(message));
    // }

    close(((fd_sv_cl)));
    return 0;
}