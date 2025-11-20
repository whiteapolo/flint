#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "config.h"

static Flint_Config *config = NULL;

Flint_Config *create_default_config()
{
  Flint_Config *config = malloc(sizeof(Flint_Config));
  config->log_statements = false;
  config->log_tokens = false;

  return config;
}

void parse_command_line_options(Flint_Config *config, int argc, char **argv)
{
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--log-statements")) {
      config->log_statements = true;
    } else if (!strcmp(argv[i], "--log-tokens")) {
      config->log_tokens = true;
    }
  }
}

void initialize_config(int argc, char **argv)
{
  config = create_default_config();
  parse_command_line_options(config, argc, argv);
}

const Flint_Config *get_config()
{
  return config;
}
