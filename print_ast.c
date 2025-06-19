#include "parser.h"
#include "print_ast.h"

void render_job(Job *job, Z_String *output);

void render_job_binary(Job_Binary *job, Z_String *output)
{
    z_str_append_format(output, "(");
    z_str_append_str(output, job->operator.lexeme);
    z_str_append_format(output, " ");
    render_job(job->left, output);
    z_str_append_format(output, " ");
    render_job(job->right, output);
    z_str_append_format(output, ")");
}

void render_job_unary(Job_Unary *job, Z_String *output)
{
    z_str_append_format(output, "(");
    z_str_append_str(output, job->operator.lexeme);
    z_str_append_format(output, " ");
    render_job(job->child, output);
    z_str_append_format(output, ")");
}

void render_job_command(Job_Command *job, Z_String *output)
{
    z_str_append_format(output, "(");

    for (int i = 1; i < job->argv.len; i++) {
        Token token = job->argv.ptr[i];
        z_str_append_format(output, " %.*s", token.lexeme.len, token.lexeme.ptr);
    }

    z_str_append_format(output, ")");
}

void render_job(Job *job, Z_String *output)
{
    if (job == NULL) {
        return;
    } else if (job->type == JOB_BINARY) {
        render_job_binary((Job_Binary *)job, output);
    } else if (job->type == JOB_UNARY) {
        render_job_unary((Job_Unary *)job, output);
    } else if (job->type == JOB_COMMAND) {
        render_job_command((Job_Command *)job, output);
    } else {
        z_str_append_format(output, "(Unknown)");
    }
}

void print_job(Job *job)
{
    Z_String s = {0};
    render_job(job, &s);

    if (s.len > 0) {
        z_str_println(Z_STR_TO_SV(s));
    }

    free(s.ptr);
}

void print_statement(Statement *statement)
{
    print_job(((Statement_Job*)(statement))->job);
}

void print_statements(Statement_Vec statements)
{
    for (int i = 0; i < statements.len; i++) {
        print_statement(statements.ptr[i]);
    }
}
