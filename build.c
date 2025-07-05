#define LIBZATAR_IMPLEMENTATION
#include "src/libzatar.h"

int main(int argc, char **argv)
{
    z_rebuild_yourself(__FILE__, argv);

    Z_Cmd cmd = {0};
    z_cmd_append(&cmd, "cc", "src/main.c", "-o", "exe");
    z_cmd_append(&cmd, "src/lexer.c", "src/token.c", "src/parser.c");
    z_cmd_append(&cmd, "src/eval.c", "src/print_ast.c", "src/environment.c");
    z_cmd_append(&cmd, "src/expantion.c", "src/interpreter.c");
    z_cmd_append(&cmd, "src/builtins/cd.c", "src/builtins/builtin.c", "src/builtins/exit.c");
    z_cmd_append(&cmd, "src/builtins/alias.c", "src/builtins/mut.c", "src/builtins/export.c");
    z_cmd_append(&cmd, "src/builtins/let.c", "src/builtins/test.c", "src/builtins/len.c");
    z_cmd_append(&cmd, "src/builtins/print.c", "src/builtins/println.c",  "src/builtins/time.c");
    z_cmd_append(&cmd, "src/builtins/command.c");
    z_cmd_append(&cmd, "src/error.c");
    z_cmd_append(&cmd, "src/scanner.c");
    z_cmd_append(&cmd, "-Wextra", "-Wall", "-Wno-unused-result");
    z_cmd_append(&cmd, "-lreadline");
    z_cmd_append(&cmd, "-g");
    z_cmd_append(&cmd, "-O0");

    return z_cmd_run_async(&cmd);
}
