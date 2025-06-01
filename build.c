#define LIBZATAR_IMPLEMENTATION
#include "libzatar.h"

int main(int argc, char **argv)
{
    z_rebuild_yourself(__FILE__, argv);

    Z_Cmd cmd;
    z_cmd_init(&cmd);
    z_cmd_append(&cmd, "cc", "main.c", "-o", "exe");
    z_cmd_append(&cmd, "lexer.c", "token.c", "parser.c", "eval.c");
    z_cmd_append(&cmd, "builtins/cd.c", "builtins/builtin.c", "builtins/exit.c");
    z_cmd_append(&cmd, "-Wextra", "-Wall", "-pedantic");
    z_cmd_append(&cmd, "-lreadline");
    z_cmd_append(&cmd, "-g");
    // z_cmd_append(&cmd, "-O3");

    return z_cmd_run_async(&cmd);
}
