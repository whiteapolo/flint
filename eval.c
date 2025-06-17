#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "libzatar.h"
#include "token.h"
#include "eval.h"
#include "builtins/builtin.h"
#include "parser.h"

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

void evaluate_command_no_fork(Ast_Node_Command *node)
{
    char **argv = node->argv;

    if (is_builtin(argv[0])) {
        exit(execute_builtin(argv));
    }

    safe_execvp(argv[0], argv);
}

int evaluate_command(Ast_Node_Command *node)
{
    char **argv = node->argv;

    if (is_builtin(argv[0])) {
        return execute_builtin(argv);
    }

    int status = 0;
    int pid = safe_fork();

    if (pid == 0) {
        safe_execvp(argv[0], argv);
    } else {
		waitpid(pid, &status, 0);
    }

    return status;
}

int evaluate_command_in_background(Ast_Node_Command *node)
{
    if (safe_fork() == 0) {
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

int evaluate_pipe(Ast_Node_Binary *node)
{
    Ast_Node_Command *command = (Ast_Node_Command *)node->left;
    Ast_Node *right = node->right;

    int fd[2];
    pipe(fd);

    int pid1 = safe_fork();

    if (pid1 == 0) {
        dup2(fd[1], STDOUT_FILENO);
        close_pipe(fd);
        evaluate_command_no_fork(command);
    }

    int pid2 = safe_fork();

    if (pid2 == 0) {
        dup2(fd[0], STDIN_FILENO);
        close_pipe(fd);
        exit(evaluate_ast(right));
    }

    close_pipe(fd);

    int status1;
    int status2;
    waitpid(pid1, &status1, 0);
    waitpid(pid2, &status2, 0);

    return status2;
}

int evaluate_and_if(Ast_Node_Binary *ast)
{
    int status = evaluate_ast(ast->left);

    if (status == 0) {
        return evaluate_ast(ast->right);
    }

    return status;
}

int evaluate_ampersand(Ast_Node_Unary *ast)
{
    return evaluate_command_in_background((Ast_Node_Command *)ast->child);
}

int evaluate_unary(Ast_Node_Unary *ast)
{
    switch (ast->operator.type) {
        case TOKEN_AMPERSAND:
            return evaluate_ampersand(ast);
        default:
            fprintf(stderr, "Unknown operator\n");
            return 0; // unreachable
    }
}

int evaluate_binary(Ast_Node_Binary *ast)
{
    switch (ast->operator.type) {
        case TOKEN_AND_IF:
            return evaluate_and_if(ast);
        case TOKEN_PIPE:
            return evaluate_pipe(ast);
        default:
            fprintf(stderr, "Unknown operator\n");
            return 0; // unreachable
    }
}

int evaluate_ast(Ast_Node *ast)
{
    if (ast == NULL) {
        return 0;
    }

    switch (ast->type) {
        case AST_NODE_COMMAND:
            return evaluate_command((Ast_Node_Command *)ast);

        case AST_NODE_UNARY:
            return evaluate_unary((Ast_Node_Unary *)ast);

        case AST_NODE_BINARY:
            return evaluate_binary((Ast_Node_Binary *)ast);
    }

    return 0;
}
