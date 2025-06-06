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

#define LIBZATAR_IMPLEMENTATION
#include "libzatar.h"

#ifndef PATH_MAX
#  define PATH_MAX 4096
#endif

static String prompt;

void update_prompt()
{
    char pwd[PATH_MAX];
    str_clear(&prompt);

    if (getcwd(pwd, PATH_MAX) == NULL) {
        str_pushf(&prompt, "couldn't retrive cwd > ");
        return;
    }

    String compressed_pwd = compress_path(CSTR_TO_SV(pwd));
    str_pushf(&prompt, COLOR_MAGENTA);
    str_push(&prompt, STR_TO_SV(compressed_pwd));
    str_pushf(&prompt, COLOR_GREEN);
    str_pushf(&prompt, " > ");
    str_pushf(&prompt, COLOR_RESET);
    str_free(&compressed_pwd);
}

void init_repl()
{
    prompt = str_new("");
    update_prompt();
}

void repl()
{
    init_repl();
    Lexer lexer;

    char *line = readline(prompt.ptr);
    add_history(line);

    while (line) {
        lexer_init(&lexer, line, strlen(line));

        if (lexer_peek(&lexer).type != TOKEN_TYPE_EOD) {
            Parser_Node *ast = parse(&lexer);
            evaluate_ast(ast);
            parser_free(ast);
        }

        update_prompt();

        free(line);
        line = readline(prompt.ptr);
        add_history(line);
    }

    free(line);
}

int main(void)
{
    repl();
    return 0;
}

void print_tokens(const char *line)
{
    Lexer lexer;
    lexer_init(&lexer, line, strlen(line));

    Token token = lexer_next(&lexer);

    while (token.type != TOKEN_TYPE_EOD && token.type != TOKEN_TYPE_ERROR) {
        print_token(token);
        token = lexer_next(&lexer);
    }

    if (token.type == TOKEN_TYPE_ERROR) {
        printf("%.*s\n", token.len, token.lexeme);
    }
}

