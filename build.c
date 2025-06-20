#define LIBZATAR_IMPLEMENTATION
#include "libzatar.h"

int main(int argc, char **argv)
{
    z_rebuild_yourself(__FILE__, argv);

    Z_Cmd cmd = {0};
    z_cmd_append(&cmd, "cc", "main.c", "-o", "exe");
    z_cmd_append(&cmd, "lexer.c", "token.c", "parser.c");
    z_cmd_append(&cmd, "eval.c");
    z_cmd_append(&cmd, "print_ast.c");
    z_cmd_append(&cmd, "environment.c");
    z_cmd_append(&cmd, "expantion.c");
    z_cmd_append(&cmd, "interpreter.c");
    z_cmd_append(&cmd, "builtins/cd.c", "builtins/builtin.c", "builtins/exit.c", "builtins/export.c");
    z_cmd_append(&cmd, "builtins/alias.c", "builtins/set.c");
    z_cmd_append(&cmd, "-Wextra", "-Wall");
    z_cmd_append(&cmd, "-lreadline");
    z_cmd_append(&cmd, "-g");
    z_cmd_append(&cmd, "-O0");

    return z_cmd_run_async(&cmd);
}
