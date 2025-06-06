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
    fprintf(stderr, "Exec failed: %s\n", strerror(errno));
    exit(1);
}

void evaluate_command_no_fork(Parser_Node *node)
{
    char **argv = node->argv;

    if (is_builtin(argv[0])) {
        exit(execute_builtin(argv));
    }

    safe_execvp(argv[0], argv);
}

int evaluate_command(Parser_Node *node)
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

int evaluate_command_in_background(Parser_Node *node)
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

int evaluate_pipe(Parser_Node *node)
{
    int len = 0;
    Parser_Node **commands = NULL;

    Parser_Node *curr_node = node;
    while (curr_node->type != NODE_TYPE_COMMAND) {
        commands = realloc(commands, sizeof(Parser_Node *) * (++len));
        commands[len - 1] = curr_node->left;
        curr_node = curr_node->right;
    }

    commands = realloc(commands, sizeof(Parser_Node *) * (++len));
    commands[len - 1] = curr_node;

    // end formating array

    int *pid = malloc(sizeof(int) * len);

    int prev[2];
    int curr[2];

    pipe(prev);

    pid[0] = safe_fork();

    if (pid[0] == 0) {
        dup2(prev[1], STDOUT_FILENO);
        close_pipe(prev);
        evaluate_command_no_fork(commands[0]);
    }

    for (int i = 1; i < len - 1; i++) {
        pipe(curr);

        pid[i] = safe_fork();

        if (pid[i] == 0) {
            dup2(curr[1], STDOUT_FILENO);
            dup2(prev[0], STDIN_FILENO);
            close_pipe(prev);
            close_pipe(curr);
            evaluate_command_no_fork(commands[i]);
        }

        close_pipe(prev);
        memcpy(prev, curr, sizeof(int) * 2);
    }

    pid[len - 1] = safe_fork();

    if (pid[len - 1] == 0) {
        dup2(prev[0], STDIN_FILENO);
        close_pipe(prev);
        evaluate_command_no_fork(commands[len - 1]);
    }

    close_pipe(prev);

    for (int i = 0; i < len - 1; i++) {
        waitpid(pid[i], NULL, 0);
    }

    int status;
    waitpid(pid[len - 1], &status, 0);

    free(pid);
    free(commands);

    return status;
}

int evaluate_ast(Parser_Node *ast)
{
    if (ast == NULL) {
        return 0;
    }

    switch (ast->type) {
        case NODE_TYPE_COMMAND:
            return evaluate_command(ast);

        case NODE_TYPE_AMPERSAND:
            return evaluate_command_in_background(ast);

        case NODE_TYPE_AND_IF: {
            int status = evaluate_ast((Parser_Node *)ast->left);

            if (status == 0) {
                return evaluate_ast((Parser_Node *)ast->right);
            }

            return status;
        }

        case NODE_TYPE_PIPE:
            return evaluate_pipe(ast);
    }

    return 0;
}
