#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "environment.h"
#include "libzatar.h"
#include "token.h"
#include "eval.h"
#include "builtins/builtin.h"
#include "parser.h"
#include "expantion.h"

int evaluate_job(Job *job);

int count_argc(char **argv)
{
    int i = 0;

    while (argv[i] != NULL) {
        i++;
    }

    return i;
}

void free_string_array(char **s)
{
    for (char **curr = s; *curr; curr++) {
        free(*curr);
    }

    free(s);
}

int safe_fork()
{
    int pid = fork();

    if (pid < 0) {
        fprintf(stderr, "Fork failed: %s\n", strerror(errno));
        fprintf(stderr, "Exit...\n");
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

void set_last_status_code(int status)
{
    char buf[16];
    snprintf(buf, 16, "%d", status);
    setenv("?", buf, 1);
}

int exec_command(char **argv)
{
    if (argv[0] == NULL) {
        return 0;
    }

    BuiltinFn fun = get_builtin(argv[0]);

    if (fun) {
        return fun(count_argc(argv), argv);
    }

    int status = 0;
    int pid = safe_fork();

    if (pid == 0) {
        safe_execvp(argv[0], argv);
    } else {
		waitpid(pid, &status, 0);
    }

    set_last_status_code(status);

    return status;
}

int evaluate_command(Job_Command *job)
{
    char **argv = expand_argv(job->argv);
    int status = exec_command(argv);

    free_string_array(argv);

    return status;
}

void close_pipe(int fd[2])
{
    close(fd[0]);
    close(fd[1]);
}

int evaluate_pipe(Job_Binary *job)
{
    Job_Command *command = (Job_Command *)job->left;
    Job *right = job->right;

    int fd[2];
    pipe(fd);

    int pid1 = safe_fork();

    if (pid1 == 0) {
        dup2(fd[1], STDOUT_FILENO);
        close_pipe(fd);
        exit(evaluate_command(command));
    }

    int pid2 = safe_fork();

    if (pid2 == 0) {
        dup2(fd[0], STDIN_FILENO);
        close_pipe(fd);
        exit(evaluate_job(right));
    }

    close_pipe(fd);

    int status1;
    int status2;
    waitpid(pid1, &status1, 0);
    waitpid(pid2, &status2, 0);

    return status2;
}

int evaluate_and(Job_Binary *job)
{
    int status = evaluate_job(job->left);
    return status ? status : evaluate_job(job->right);
}

int evaluate_or(Job_Binary *job)
{
    int status = evaluate_job(job->left);
    return status ? evaluate_job(job->right) : status;
}

int evaluate_ampersand(Job_Unary *job)
{
    if (safe_fork() == 0) {
        int status = evaluate_job(job->child);

        if (status != 0) {
            fprintf(stderr, "exit with status %d\n", status);
        } else {
            printf("done\n");
        }

        exit(status);
    }

    return 0;
}

int evaluate_unary(Job_Unary *job)
{
    switch (job->operator.type) {
        case TOKEN_AMPERSAND:
            return evaluate_ampersand(job);
        default:
            assert(0 && "Unknown operator\n");
            return 0; // unreachable
    }
}

int evaluate_binary(Job_Binary *job)
{
    switch (job->operator.type) {
        case TOKEN_AND:
            return evaluate_and(job);
        case TOKEN_OR:
            return evaluate_or(job);
        case TOKEN_PIPE:
            return evaluate_pipe(job);
        default:
            assert(0 && "Unknown operator\n");
            return 0; // unreachable
    }
}

int evaluate_job(Job *job)
{
    if (job == NULL) {
        return 0;
    }

    switch (job->type) {
        case JOB_COMMAND:
            return evaluate_command((Job_Command *)job);

        case JOB_UNARY:
            return evaluate_unary((Job_Unary *)job);

        case JOB_BINARY:
            return evaluate_binary((Job_Binary *)job);
    }

    return 0;
}

void evaluate_if(Statement_If *statement)
{
    extern Environment environment;
    Environment previous_envirnoment = environment;
    environment = environment_new(&previous_envirnoment);

    if (!evaluate_job(statement->condition)) {
        evaluate_statements(&statement->ifBranch);
    } else {
        evaluate_statements(&statement->elseBranch);
    }

    environment_free(&environment);
    environment = previous_envirnoment;
}

int evaluate_statement(Statement *statement)
{
    switch (statement->type) {
        case STATEMENT_JOB:
            return evaluate_job(((Statement_Job *)statement)->job);

        case STATEMENT_IF:
            evaluate_if(((Statement_If *)statement));
            return 0;

        default:
            return 0;
    }
}

void evaluate_statements(const Statement_Vec *statements)
{
    for (int i = 0; i < statements->len; i++) {
        evaluate_statement(statements->ptr[i]);
    }
}
