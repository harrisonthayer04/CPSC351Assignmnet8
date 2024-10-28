#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

extern char **environ;

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <inp> <cmd> <out>\n", argv[0]);
        return 1;
    }

    int fd_in = STDIN_FILENO, fd_out = STDOUT_FILENO;

    // Input redirection
    if (strcmp(argv[1], "-") != 0) {
        fd_in = open(argv[1], O_RDONLY);
        if (fd_in == -1) {
            perror("Failed to open input file");
            return 1;
        }
    }

    // Output redirection
    if (strcmp(argv[3], "-") != 0) {
        fd_out = open(argv[3], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_out == -1) {
            perror("Failed to open output file");
            if (fd_in != STDIN_FILENO) close(fd_in);
            return 1;
        }
    }

    char *cmd = argv[2];
    char *args[1024]; // Maximum of 1024 arguments
    int count = 0;

    char *temp = strtok(cmd, " ");
    while (temp != NULL && count < 63) {
        args[count++] = temp;
        temp = strtok(NULL, " ");
    }
    args[count] = NULL; 

    // Find the path
    char *cmd_path = NULL;
    if (strchr(args[0], '/')) {
        cmd_path = args[0];
    } else {
        char *path_env = getenv("PATH");
        char *path1 = strdup(path_env);
        char *pathToken = strtok(path1, ":");
        while (pathToken != NULL) {
            char completePath[1024];
            snprintf(completePath, sizeof(completePath), "%s/%s", pathToken, args[0]);
            if (access(completePath, X_OK) == 0) {
                cmd_path = strdup(completePath);
                break;
            }
            pathToken = strtok(NULL, ":");
        }
        free(path1);
    }
    pid_t child_pid = fork();

    if (child_pid == 0) {
        if (fd_in != STDIN_FILENO) {
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }
        if (fd_out != STDOUT_FILENO) {
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }
        execve(cmd_path, args, environ);
        perror("Failed to execute command");
        exit(1);
    }

    // Parent process
    if (fd_in != STDIN_FILENO) close(fd_in);
    if (fd_out != STDOUT_FILENO) close(fd_out);

    wait(NULL);
    printf("Parent pid is %d. Forked child pid %d. Parent exiting\n", getpid(), child_pid);

    if (cmd_path != args[0]) free(cmd_path);

    return 0;
}