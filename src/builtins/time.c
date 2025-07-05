#include <time.h>
#include <stdio.h>
#include "../eval.h"
#include <sys/time.h>

int builtin_time(int argc, char **argv)
{
    if (argc == 1) {
        fprintf(stderr, "Usage: time <command>\n");
        return 1;
    }

    struct timeval start;
    struct timeval end;

    gettimeofday(&start, NULL);
    int ret = exec_command(argv + 1);
    gettimeofday(&end, NULL);

    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

    printf("%lf\n", elapsed_time);

    return ret;
}
