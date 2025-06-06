#define LIBZATAR_IMPLEMENTATION
#include "libzatar.h"

int main(int argc, char **argv)
{
    rebuild_yourself(__FILE__, argv);

    Cmd cmd;
    cmd_init(&cmd);
    cmd_append(&cmd, "cc", "main.c", "-o", "exe");
    cmd_append(&cmd, "lexer.c", "token.c", "parser.c", "eval.c");
    cmd_append(&cmd, "builtins/cd.c", "builtins/builtin.c", "builtins/exit.c");
    cmd_append(&cmd, "-Wextra", "-Wall", "-pedantic");
    cmd_append(&cmd, "-lreadline");
    cmd_append(&cmd, "-g");
    // cmd_append(&cmd, "-O3");

    return z_cmd_run_async(&cmd);
}
