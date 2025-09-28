#include <dirent.h>
#include <unistd.h>
#define LIBZATAR_IMPLEMENTATION
#include "src/libzatar.h"

void append_folder(Z_Cmd *cmd, const char *dirname)
{
  DIR *dir = opendir(dirname);
  if (!dir)
    return;

  struct dirent *de;

  Z_String path = {0};

  while ((de = readdir(dir))) {
    if (z_sv_ends_with(Z_CSTR(de->d_name), Z_CSTR(".c"))) {
      path.len = 0;
      z_str_append_format(&path, "%s/%s", dirname, de->d_name);
      z_cmd_append(cmd, z_str_to_cstr(&path));
    }
  }

  z_str_free(&path);
  closedir(dir);
}

int main(int argc, char **argv)
{
  z_rebuild_yourself(__FILE__, argv);

  Z_Cmd cmd = {0};
  z_cmd_append(&cmd, "cc", "-o", "exe");
  append_folder(&cmd, "src/builtins");
  append_folder(&cmd, "src");
  z_cmd_append(&cmd, "-Wextra", "-Wall", "-Wno-unused-result");
  z_cmd_append(&cmd, "-lreadline");
  z_cmd_append(&cmd, "-g");
  z_cmd_append(&cmd, "-O0");

  return z_cmd_run_sync(&cmd);
}
