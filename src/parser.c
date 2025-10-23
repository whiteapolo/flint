#include "parser.h"
#include "cstr.h"
#include "error.h"
#include "lexer.h"
#include "libzatar.h"
#include "token.h"
#include <assert.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void free_job(Job *job);
void free_statements(Statement_Vec *statements);
Statement *parse_statement();

typedef struct {
  const Token_Vec *tokens;
  int curr;
  bool had_error;
  bool panic_mode;
  const char *source;
  char **source_by_lines;
} Parser_State;

Parser_State *parser_state = NULL;

void create_new_parser_state(const Token_Vec *tokens, const char *source)
{
  parser_state = malloc(sizeof(Parser_State));
  parser_state->tokens = tokens;
  parser_state->curr = 0;
  parser_state->had_error = false;
  parser_state->panic_mode = false;
  parser_state->source = source;
  parser_state->source_by_lines = str_split(source, "\n");
}

void free_parser_state()
{
  free(parser_state->source_by_lines);
  free(parser_state);
}

static Token advance()
{
  assert(parser_state->curr < parser_state->tokens->len);
  return parser_state->tokens->ptr[parser_state->curr++];
}

static Token peek()
{
  assert(parser_state->curr < parser_state->tokens->len);
  return parser_state->tokens->ptr[parser_state->curr];
}

static bool is_at_end()
{
  return peek().type == TOKEN_EOD;
}

static bool check(Token_Type type)
{
  if (parser_state->panic_mode) {
    return false;
  }

  return peek().type == type;
}

static bool check_array(Token_Type types[], int len)
{
  for (int i = 0; i < len; i++) {
    if (check(types[i])) {
      return true;
    }
  }

  return false;
}

static bool check_keyword()
{
  Token_Type keywords[] = {
    TOKEN_FOR, TOKEN_IN, TOKEN_FUN, TOKEN_END, TOKEN_ELSE, TOKEN_WHILE,
  };

  return check_array(keywords, Z_ARRAY_LEN(keywords));
}

static bool check_string()
{
  Token_Type string_types[] = {
    TOKEN_WORD,
    TOKEN_DQUOTED_STRING,
    TOKEN_SQUOTED_STRING,
  };

  return check_array(string_types, Z_ARRAY_LEN(string_types));
}

static bool check_argument() {
  // argument override keywords
  // for example:
  // echo if
  return check_string() || check_keyword();
}

static bool match(Token_Type type)
{
  if (parser_state->had_error) {
    return false;
  }

  if (check(type)) {
    advance();
    return true;
  }

  return false;
}

static void error(Token token, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);

  if (!parser_state->panic_mode) {
    syntax_error_at_token_va((const char *const *)parser_state->source_by_lines, token, fmt, ap);
  }

  parser_state->had_error = true;
  parser_state->panic_mode = true;
  va_end(ap);
}

Token consume(Token_Type type, const char *msg)
{
  if (check(type)) {
    return advance();
  }

  error(peek(), msg);

  return peek();
}

Token consume_string(const char *msg)
{
  if (check_string()) {
    return advance();
  }

  error(peek(), msg);

  return peek();
}

Token consume_argument(const char *msg)
{
  if (check_argument()) {
    return advance();
  }

  error(peek(), msg);

  return peek();
}

void skip_empty_statements()
{
  while (!is_at_end() && match(TOKEN_STATEMENT_END)) { }
}

void synchronize()
{
  skip_empty_statements();
  while (!is_at_end() && advance().type != TOKEN_STATEMENT_END) { }
  parser_state->panic_mode = false;
}

Statement *create_statement_if(Job *condition, Statement_Vec ifBranch, Statement_Vec elseBranch)
{
  Statement_If *node = malloc(sizeof(Statement_If));
  node->type = STATEMENT_IF;
  node->condition = condition;
  node->ifBranch = ifBranch;
  node->elseBranch = elseBranch;

  return (Statement *)node;
}

Statement *create_statement_while(Job *condition, Statement_Vec body)
{
  Statement_While *node = malloc(sizeof(Statement_While));
  node->type = STATEMENT_WHILE;
  node->condition = condition;
  node->body = body;

  return (Statement *)node;
}

Statement *create_statement_function(Token name, Statement_Vec body)
{
  Statement_Function *node = malloc(sizeof(Statement_Function));
  node->type = STATEMENT_FUNCTION;
  node->body = body;
  node->name = clone_token(name);

  return (Statement *)node;
}

Statement *create_statement_for(Token var_name, Token string, Token delim, Statement_Vec body)
{
  Statement_For *node = malloc(sizeof(Statement_For));
  node->type = STATEMENT_FOR;
  node->body = body;
  node->var_name = clone_token(var_name);
  node->string = clone_token(string);
  node->delim = clone_token(delim);

  return (Statement *)node;
}

Statement *create_statement_job(Job *job)
{
  Statement_Job *node = malloc(sizeof(Statement_Job));
  node->type = STATEMENT_JOB;
  node->job = job;

  return (Statement *)node;
}

Job *create_binary(Job *left, Token operator, Job * right)
{
  Job_Binary *node = malloc(sizeof(Job_Binary));
  node->type = JOB_BINARY;
  node->left = left;
  node->operator= clone_token(operator);
  node->right = right;

  return (Job *)node;
}

Job *create_unary(Token operator, Job * child)
{
  Job_Unary *node = malloc(sizeof(Job_Unary));
  node->type = JOB_UNARY;
  node->operator= clone_token(operator);
  node->child = child;

  return (Job *)node;
}

Job *create_command(Token_Vec argv)
{
  Job_Command *node = malloc(sizeof(Job_Command));
  node->type = JOB_COMMAND;
  node->argv = argv;

  return (Job *)node;
}

Job *parse_simple_command()
{
  Token_Vec argv = {0};

  while (check_argument()) {
    Token token = advance();
    z_da_append(&argv, clone_token(token));
  }

  if (argv.len == 0) {
    error(peek(), "Expected command.");
  }

  return create_command(argv);
}

Job *parse_pipeline()
{
  Job *job = parse_simple_command();

  while (check(TOKEN_PIPE)) {
    Token pipe = advance();
    Job *right = parse_simple_command();
    job = create_binary(job, pipe, right);
  }

  return job;
}

Job *parse_and()
{
  Job *job = parse_pipeline();

  while (check(TOKEN_AND)) {
    Token and_if = advance();
    Job *right = parse_pipeline();
    job = create_binary(job, and_if, right);
  }

  return job;
}

Job *parse_or()
{
  Job *job = parse_and();

  while (check(TOKEN_OR)) {
    Token or = advance();
    Job *right = parse_and();
    job = create_binary(job, or, right);
  }

  return job;
}

Job *parse_background_job()
{
  Job *job = parse_or();

  if (check(TOKEN_AMPERSAND)) {
    Token ampersand = advance();
    return create_unary(ampersand, job);
  }

  return job;
}

Job *parse_job()
{
  return parse_background_job();
}

Statement *parse_job_statement()
{
  Job *job = parse_job();
  return job ? create_statement_job(job) : NULL;
}

Statement_Vec parse_block_untill(Token_Type types[], int len)
{
  Statement_Vec statements = {0};
  skip_empty_statements();

  while (!is_at_end() && !check_array(types, len)) {

    z_da_append(&statements, parse_statement());

    if (parser_state->panic_mode) {
      synchronize();
    }

    skip_empty_statements();
  }

  return statements;
}

Statement_Vec parse_block_untill_end()
{
  Token_Type end[] = {TOKEN_END};
  return parse_block_untill(end, Z_ARRAY_LEN(end));
}

Statement *parse_if_statement()
{
  Token_Type if_body_end[] = {TOKEN_ELSE, TOKEN_END};

  Job *condition = parse_job();

  skip_empty_statements();

  Statement_Vec ifBranch = parse_block_untill(if_body_end, Z_ARRAY_LEN(if_body_end));
  Statement_Vec elseBranch = {0};

  skip_empty_statements();

  if (match(TOKEN_ELSE)) {
    elseBranch = parse_block_untill_end();
  }

  skip_empty_statements();

  consume(TOKEN_END, "Expected 'end' after if statement");

  return create_statement_if(condition, ifBranch, elseBranch);
}

Statement *parse_while_statement()
{
  Job *condition = parse_job();

  skip_empty_statements();

  Statement_Vec body = parse_block_untill_end();

  consume(TOKEN_END, "Expected 'end' after while statement");

  return create_statement_while(condition, body);
}

Statement *parse_for_statement()
{
  Token var_name = consume_string("Expected idenifier after for.");
  consume(TOKEN_IN, "Expected 'in' after idenifier.");

  Token string = consume_string("Expected string after in.");
  consume(TOKEN_BY, "Expected 'by' after idenifier.");

  Token delim = consume_string("Expected delimeter string after by.");

  skip_empty_statements();

  Statement_Vec body = parse_block_untill_end();
  consume(TOKEN_END, "Expected 'end' after if statement");

  return create_statement_for(var_name, string, delim, body);
}

Statement *parse_function_statement()
{
  Token name = consume(TOKEN_WORD, "Expected function name after 'fn'");

  skip_empty_statements();

  Statement_Vec body = parse_block_untill_end();

  consume(TOKEN_END, "Expected 'end' after function body.");

  return create_statement_function(name, body);
}

Statement *parse_statement()
{
  if (match(TOKEN_IF)) return parse_if_statement();
  if (match(TOKEN_WHILE)) return parse_while_statement();
  if (match(TOKEN_FOR)) return parse_for_statement();
  if (match(TOKEN_FUN)) return parse_function_statement();
  return parse_job_statement();
}

Statement_Vec parse(const Token_Vec *tokens, const char *source)
{
  create_new_parser_state(tokens, source);
  Statement_Vec statements = parse_block_untill(NULL, 0);

  if (parser_state->had_error) {
    parser_free(&statements);
    free_parser_state();
    return (Statement_Vec){0};
  }

  free_parser_state();
  return statements;
}

void free_job_command(Job_Command *cmd)
{
  free_tokens(&cmd->argv);
  free(cmd);
}

void free_job_unary(Job_Unary *un)
{
  free_token(&un->operator);
  free_job(un->child);
  free(un);
}

void free_job_binary(Job_Binary *bin)
{
  free_token(&bin->operator);
  free_job(bin->left);
  free_job(bin->right);
  free(bin);
}

void free_job(Job *job)
{
  if (job == NULL) {
    return;
  }

  switch (job->type) {
    case JOB_COMMAND:
      free_job_command((Job_Command *)job);
      break;

    case JOB_UNARY:
      free_job_unary((Job_Unary *)job);
      break;

    case JOB_BINARY:
      free_job_binary((Job_Binary *)job);
      break;
  }
}

void free_if_statement(Statement_If *statement)
{
  free_job(statement->condition);
  free_statements(&statement->ifBranch);
  free(statement);
}

void free_while_statement(Statement_While *statement)
{
  free_job(statement->condition);
  free_statements(&statement->body);
  free(statement);
}

void free_for_statement(Statement_For *statement)
{
  free_statements(&statement->body);
  free(statement);
}

void free_function_statement(Statement_Function *statement)
{
  free_statements(&statement->body);
  free_token(&statement->name);
  free(statement);
}

void free_job_statement(Statement_Job *statement)
{
  Job *job = statement->job;
  free_job(job);
  free(statement);
}

void free_statement(Statement *statement)
{
  switch (statement->type) {
    case STATEMENT_JOB:
      free_job_statement((Statement_Job *)statement);
      break;

    case STATEMENT_IF:
      free_if_statement((Statement_If *)statement);
      break;

    case STATEMENT_WHILE:
      free_while_statement((Statement_While *)statement);
      break;

    case STATEMENT_FOR:
      free_for_statement((Statement_For *)statement);
      break;

    case STATEMENT_FUNCTION:
      free_function_statement((Statement_Function *)statement);
      break;
  }
}

void free_statements(Statement_Vec *statements)
{
  for (int i = 0; i < statements->len; i++) {
    free_statement(statements->ptr[i]);
  }

  free(statements->ptr);
}

void parser_free(Statement_Vec *statements)
{
  free_statements(statements);
}

Statement *clone_statement(Statement *statement)
{
  switch (statement->type) {
    case STATEMENT_FUNCTION: return clone_statement_function((const Statement_Function *)statement);
    case STATEMENT_FOR: return clone_statement_for((const Statement_For *)statement);
    case STATEMENT_IF: return clone_statement_if((const Statement_If *)statement);
    case STATEMENT_WHILE: return clone_statement_while((const Statement_While *)statement);
    case STATEMENT_JOB: return clone_statement_job((const Statement_Job *)statement);
    default: return NULL;
  }
}

Statement_Vec clone_statements(Statement_Vec statements)
{
  Statement_Vec new_statements = {0};

  z_da_foreach(Statement **, statement, &statements) {
    z_da_append(&new_statements, clone_statement(*statement));
  }

  return new_statements;
}

Job *clone_job(const Job *job)
{
  switch (job->type) {
    case JOB_BINARY: return clone_job_binary((const Job_Binary *)job);
    case JOB_UNARY: return clone_job_unary((const Job_Unary *)job);
    case JOB_COMMAND: return clone_job_command((const Job_Command *)job);
    default: return NULL;
  }
}

Job *clone_job_binary(const Job_Binary *job)
{
  return create_binary(clone_job(job->left), clone_token(job->operator), clone_job(job->right));
}

Job *clone_job_unary(const Job_Unary *job)
{
  return create_unary(clone_token(job->operator), clone_job(job->child));
}

Job *clone_job_command(const Job_Command *job)
{
  return create_command(clone_tokens(job->argv));
}

Statement *clone_statement_function(const Statement_Function *fn)
{
  return create_statement_function(fn->name, clone_statements(fn->body));
}

Statement *clone_statement_if(const Statement_If *statement)
{
  return create_statement_if(
      clone_job(statement->condition),
      clone_statements(statement->ifBranch),
      clone_statements(statement->elseBranch)
  );
}

Statement *clone_statement_for(const Statement_For *statement)
{
  return create_statement_for(
      clone_token(statement->var_name),
      clone_token(statement->string),
      clone_token(statement->delim),
      clone_statements(statement->body)
  );
}

Statement *clone_statement_job(const Statement_Job *statement)
{
  return create_statement_job(clone_job(statement->job));
}

Statement *clone_statement_while(const Statement_While *statement)
{
  return create_statement_while(clone_job(statement->condition), clone_statements(statement->body));
}
