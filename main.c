#include <sys/ucontext.h>
#define LIBZATAR_IMPLEMENTATION
#include "libzatar.h"

#include "lexer.h"
#include "parser.h"
#include "token.h"
#include "eval.h"
#include <ctype.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "builtins/alias.h"

#define INIT_FILE_PATH "~/.config/flint/flint.rc"

#ifndef PATH_MAX
#  define PATH_MAX 4096
#endif

static Z_String prompt = {0};

void update_prompt()
{
    char pwd[PATH_MAX];
    z_str_clear(&prompt);

    if (getcwd(pwd, PATH_MAX) == NULL) {
        z_str_append_format(&prompt, "couldn't retrive cwd > ");
        return;
    }

    Z_String compressed_pwd = z_compress_path(Z_CSTR_TO_SV(pwd));
    z_str_append_format(&prompt, Z_COLOR_MAGENTA);
    z_str_append_str(&prompt, Z_STR_TO_SV(compressed_pwd));
    z_str_append_format(&prompt, Z_COLOR_GREEN);
    z_str_append_format(&prompt, " > ");
    z_str_append_format(&prompt, Z_COLOR_RESET);
    z_str_free(&compressed_pwd);
}

void execute_line(Z_String_View line)
{
    Token_Vec tokens = lexer_get_tokens(line);
    alias_expension(&tokens);
    // lexer_print_tokens(&tokens);
    Ast_Node *ast = parse(&tokens);
    // print_ast(ast);
    evaluate_ast(ast);
    parser_free(ast);
    free(tokens.ptr);
}

void repl()
{
    char *line;
    update_prompt();

    while ((line = readline(z_str_to_cstr(&prompt)))) {
        add_history(line);
        execute_line(Z_CSTR_TO_SV(line));
        update_prompt();
        free(line);
    }
}

void run_file_content(Z_String_View file_content)
{
    Z_String_View delim = Z_CSTR_TO_SV("\n");
    Z_String_View line = z_str_tok_start(file_content, delim);

    while (line.len > 0) {
        execute_line(line);
        line = z_str_tok_next(Z_STR_TO_SV(file_content), line, delim);
    }
}

void execute_file_from_raw_path(const char *pathname)
{
    Z_String file_content = {0};

    if (!z_read_whole_file(pathname, &file_content)) {
        z_print_warning("No such file or directory: '%s'\n", pathname);
        return;
    }

    run_file_content(Z_STR_TO_SV(file_content));
    z_str_free(&file_content);
}

void execute_file(Z_String_View pathname)
{
    Z_String expanded_path = {0};
    z_expand_path(pathname, &expanded_path);
    execute_file_from_raw_path(z_str_to_cstr(&expanded_path));
    z_str_free(&expanded_path);
}

void execute_init_file()
{
    Z_String_View init_file = Z_CSTR_TO_SV(INIT_FILE_PATH);
    execute_file(init_file);
}

int main(int argc, char **argv)
{
    execute_init_file();

    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        execute_file(Z_CSTR_TO_SV(argv[1]));
    } else {
        z_die_format("Usage: Flint <path>\n");
    }
}
