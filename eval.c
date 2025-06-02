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

int evaluate_command(Parser_Node *node)
{
    char **argv = node->command.argv;

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

int evaluate_pipe(Parser_Node *node)
{
    Parser_Node *command = (Parser_Node *)node->binary.right;
    Parser_Node *left = (Parser_Node *)node->binary.left;

    if (left->type == NODE_TYPE_PIPE) {
        assert(0 && "nesting pipes arent supported yet");
    }

    assert(left->type == NODE_TYPE_COMMAND);

    int fd[2];
    pipe(fd);

    int pid1 = safe_fork();

    if (pid1 == 0) {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        exit(evaluate_command(left));
    }

    int pid2 = safe_fork();

    if (pid2 == 0) {
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        close(fd[1]);
        exit(evaluate_command(command));
    }

    close(fd[0]);
    close(fd[1]);

    int status1;
    int status2;
    waitpid(pid1, &status1, 0);
    waitpid(pid2, &status2, 0);

    return status2;
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
            return evaluate_command_in_background((Parser_Node *)ast->unary.child);

        case NODE_TYPE_AND_IF: {
            int status = evaluate_ast((Parser_Node *)ast->binary.left);

            if (status == 0) {
                return evaluate_ast((Parser_Node *)ast->binary.right);
            }

            return status;
        }

        case NODE_TYPE_PIPE:
            return evaluate_pipe(ast);
    }

    return 0;
}
