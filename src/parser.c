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

Statement *parse_statement();

typedef struct {
  const Token_Array *tokens;
  int curr;
  bool had_error;
  bool panic_mode;
  const char *source;
  char **source_by_lines;
} Parser_State;

static Parser_State *parser_state = NULL;

void parser_init(const Token_Array *tokens, const char *source)
{
  parser_state = malloc(sizeof(Parser_State));
  parser_state->tokens = tokens;
  parser_state->curr = 0;
  parser_state->had_error = false;
  parser_state->panic_mode = false;
  parser_state->source = source;
  parser_state->source_by_lines = str_split(source, "\n");
}

void parser_free()
{
  str_free_array(parser_state->source_by_lines);
  free(parser_state);
  parser_state = NULL;
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

static bool check_array(Token_Type *types, int len)
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
#define X(type, lexeme, is_keyword) \
  if (is_keyword && check(type)) {  \
     return true;                   \
  }
    TOKEN_TYPES
#undef X
  return false;
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

static void parser_error(Token token, const char *fmt, ...)
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

  parser_error(peek(), msg);

  return peek();
}

Token consume_string(const char *msg)
{
  if (check_string()) {
    return advance();
  }

  parser_error(peek(), msg);

  return peek();
}

Token consume_argument(const char *msg)
{
  if (check_argument()) {
    return advance();
  }

  parser_error(peek(), msg);

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

Job *parse_simple_command()
{
  Token_Array argv = {0};

  while (check_argument()) {
    Token token = advance();
    z_da_append(&argv, clone_token(token));
  }

  if (argv.len == 0) {
    parser_error(peek(), "Expected command.");
  }

  return create_job_command(argv);
}

Job *parse_pipeline()
{
  Job *job = parse_simple_command();

  while (check(TOKEN_PIPE)) {
    Token pipe = advance();
    Job *right = parse_simple_command();
    job = create_job_binary(job, clone_token(pipe), right);
  }

  return job;
}

Job *parse_and()
{
  Job *job = parse_pipeline();

  while (check(TOKEN_AND)) {
    Token and_if = advance();
    Job *right = parse_pipeline();
    job = create_job_binary(job, clone_token(and_if), right);
  }

  return job;
}

Job *parse_or()
{
  Job *job = parse_and();

  while (check(TOKEN_OR)) {
    Token or = advance();
    Job *right = parse_and();
    job = create_job_binary(job, clone_token(or), right);
  }

  return job;
}

Job *parse_background_job()
{
  Job *job = parse_or();

  if (check(TOKEN_AMPERSAND)) {
    Token ampersand = advance();
    return create_job_unary(clone_token(ampersand), job);
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

Statement_Array parse_block_until(Token_Type types[], int len)
{
  Statement_Array statements = {0};
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

Statement_Array parse_block_until_end()
{
  Token_Type end[] = { TOKEN_END };
  return parse_block_until(end, Z_ARRAY_LEN(end));
}

Statement *parse_if_statement()
{
  Token_Type if_body_end[] = {TOKEN_ELSE, TOKEN_END};

  Job *condition = parse_job();

  skip_empty_statements();

  Statement_Array ifBranch = parse_block_until(if_body_end, Z_ARRAY_LEN(if_body_end));
  Statement_Array elseBranch = {0};

  skip_empty_statements();

  if (match(TOKEN_ELSE)) {
    elseBranch = parse_block_until_end();
  }

  skip_empty_statements();

  consume(TOKEN_END, "Expected 'end' after if statement");

  return create_statement_if(condition, ifBranch, elseBranch);
}

Statement *parse_while_statement()
{
  Job *condition = parse_job();

  skip_empty_statements();

  Statement_Array body = parse_block_until_end();

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

  Statement_Array body = parse_block_until_end();
  consume(TOKEN_END, "Expected 'end' after if statement");

  return create_statement_for(clone_token(var_name), clone_token(string), clone_token(delim), body);
}

Statement *parse_function_statement()
{
  Token name = consume(TOKEN_WORD, "Expected function name after 'fn'");

  skip_empty_statements();

  Statement_Array body = parse_block_until_end();

  consume(TOKEN_END, "Expected 'end' after function body.");

  return create_statement_function(clone_token(name), body);
}

Statement *parse_statement()
{
  if (match(TOKEN_IF))    return parse_if_statement();
  if (match(TOKEN_FOR))   return parse_for_statement();
  if (match(TOKEN_WHILE)) return parse_while_statement();
  if (match(TOKEN_FUN))   return parse_function_statement();
  return parse_job_statement();
}

Statement_Array parse(const Token_Array *tokens, const char *source)
{
  parser_init(tokens, source);
  Statement_Array statements = parse_block_until(NULL, 0);

  if (parser_state->had_error) {
    free_statements(&statements);
    parser_free();
    return (Statement_Array){0};
  }

  parser_free();
  return statements;
}
