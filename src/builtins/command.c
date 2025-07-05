#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int builtin_command(int argc, char **argv)
{
    if (argc == 1) {
        fprintf(stderr, "Usage: command <commands>\n");
        return 1;
    }

    int status = 0;
    int pid = fork();

    if (pid < 0) {
        fprintf(stderr, "Fork failed\n");
    } else if (pid == 0) {
        execvp(argv[1], argv + 1);
    } else {
        waitpid(pid, &status, 0);
        return status;
    }

    return status;
}
