#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
  bool log_tokens;
  bool log_statements;
  const char *init_file_path;
} Flint_Config;

void initialize_config(int argc, char **argv);
const Flint_Config *get_config();

#endif
