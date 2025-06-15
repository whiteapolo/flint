#include "eval.h"
#include "builtins/builtin.h"
#include "parser.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "libzatar.h"
#include "token.h"

int safe_fork()
{
    int pid = fork();

    if (pid < 0) {
        fprintf(stderr, "Fork failed\n");
        fprintf(stderr, "Exisiting...\n");
        exit(1);
    }

    return pid;
}

void safe_execvp(const char *file, char *const argv[])
{
    execvp(file, argv);
    fprintf(stderr, "'%s': %s\n", file, strerror(errno));
    exit(1);
}

void evaluate_command_no_fork(Parser_Node_Command *node)
{
    char **argv = node->argv;

    if (is_builtin(argv[0])) {
        exit(execute_builtin(argv));
    }

    safe_execvp(argv[0], argv);
}

int evaluate_command(Parser_Node_Command *node)
{
    char **argv = node->argv;

    if (is_builtin(argv[0])) {
        return execute_builtin(argv);
    }

    int status = 0;
    int pid = safe_fork();

    if (pid == 0) { // child
        safe_execvp(argv[0], argv);
    } else { // parent
		waitpid(pid, &status, 0);
    }

    return status;
}

int evaluate_command_in_background(Parser_Node_Command *node)
{
    if (safe_fork() == 0) { // child
        int status = evaluate_command(node);

        if (status != 0) {
            fprintf(stderr, "exit with status %d\n", status);
        } else {
            printf("done\n");
        }

        exit(status);
    }

    return 0;
}

void close_pipe(int fd[2])
{
    close(fd[0]);
    close(fd[1]);
}

int evaluate_pipe(Parser_Node_Binary *node)
{
    typedef struct {
        Parser_Node_Command **ptr;
        int len;
        int capacity;
    } Command_Vec;

    Command_Vec commands = {0};

    Parser_Node_Binary *curr_node = node;

    while (curr_node->type == Parser_Node_Type_BINARY) {
        z_da_append(&commands, (Parser_Node_Command *)curr_node->left);
        curr_node = (Parser_Node_Binary *)curr_node->right;
    }

    z_da_append(&commands, (Parser_Node_Command *)curr_node);

    // end formating array

    int *pid = malloc(sizeof(int) * commands.len);

    int prev[2];
    int curr[2];

    pipe(prev);

    pid[0] = safe_fork();

    if (pid[0] == 0) {
        dup2(prev[1], STDOUT_FILENO);
        close_pipe(prev);
        evaluate_command_no_fork(commands.ptr[0]);
    }

    for (int i = 1; i < commands.len - 1; i++) {
        pipe(curr);

        pid[i] = safe_fork();

        if (pid[i] == 0) {
            dup2(curr[1], STDOUT_FILENO);
            dup2(prev[0], STDIN_FILENO);
            close_pipe(prev);
            close_pipe(curr);
            evaluate_command_no_fork(commands.ptr[i]);
        }

        close_pipe(prev);
        memcpy(prev, curr, sizeof(int) * 2);
    }

    pid[commands.len - 1] = safe_fork();

    if (pid[commands.len - 1] == 0) {
        dup2(prev[0], STDIN_FILENO);
        close_pipe(prev);
        evaluate_command_no_fork(commands.ptr[commands.len - 1]);
    }

    close_pipe(prev);

    for (int i = 0; i < commands.len - 1; i++) {
        waitpid(pid[i], NULL, 0);
    }

    int status;
    waitpid(pid[commands.len - 1], &status, 0);

    free(pid);
    free(commands.ptr);

    return status;
}

int evaluate_and_if(Parser_Node_Binary *ast)
{
    int status = evaluate_ast(ast->left);

    if (status == 0) {
        return evaluate_ast(ast->right);
    }

    return status;
}

int evaluate_ampersand(Parser_Node_Unary *ast)
{
    return evaluate_command_in_background((Parser_Node_Command *)ast->child);
}

int evaluate_unary(Parser_Node_Unary *ast)
{
    switch (ast->operator.type) {
        case TOKEN_TYPE_AMPERSAND:
            return evaluate_ampersand(ast);
        default:
            fprintf(stderr, "Unknown operator\n");
            return 0; // unreachable
    }
}

int evaluate_binary(Parser_Node_Binary *ast)
{
    switch (ast->operator.type) {
        case TOKEN_TYPE_AND_IF:
            return evaluate_and_if(ast);
        case TOKEN_TYPE_PIPE:
            return evaluate_pipe(ast);
        default:
            fprintf(stderr, "Unknown operator\n");
            return 0; // unreachable
    }
}

int evaluate_ast(Parser_Node *ast)
{
    if (ast == NULL) {
        return 0;
    }

    switch (ast->type) {
        case Parser_Node_Type_COMMAND:
            return evaluate_command((Parser_Node_Command *)ast);

        case Parser_Node_Type_UNARY:
            return evaluate_unary((Parser_Node_Unary *)ast);

        case Parser_Node_Type_BINARY:
            return evaluate_binary((Parser_Node_Binary *)ast);
    }

    return 0;
}
