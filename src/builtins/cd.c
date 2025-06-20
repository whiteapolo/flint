#include "cd.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char *get_home()
{
    const char *home = getenv("HOME");

    if (home == NULL) {
        return ".";
    }

    return home;
}

int builtin_cd_home()
{
    int status = chdir(get_home());

    if (status != 0) {
        fprintf(stderr, "builtin_cd: %s\n", strerror(errno));
    }

    return status;
}


int builtin_cd_path(const char *pathname)
{
    int status = chdir(pathname);

    if (status != 0) {
        fprintf(stderr, "builtin_cd: %s\n", strerror(errno));
    }

    return status;
}

int builtin_cd(int argc, char **argv)
{
    if (argc == 1) {
        return builtin_cd_home();
    } else if (argc == 2) {
        return builtin_cd_path(argv[1]);
    } else {
        fprintf(stderr, "builtin_cd: accept only one argument\n");
        exit(1);
    }
}
