#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "config.h"

static Flint_Config *config = NULL;

Flint_Config *create_default_config()
{
  Flint_Config *config = malloc(sizeof(Flint_Config));
  config->help = false;
  config->log_statements = false;
  config->log_tokens = false;
  config->init_file_path = "~/.config/flint/init.flint";

  return config;
}

void parse_command_line_options(Flint_Config *config, int argc, char **argv)
{
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--log-statements")) {
      config->log_statements = true;
    } else if (!strcmp(argv[i], "--log-tokens")) {
      config->log_tokens = true;
    } else if (!strcmp(argv[i], "-h")) {
      config->help = true;
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
