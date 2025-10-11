#ifndef CSTR_H
#define CSTR_H

int str_get_format_size(const char *fmt, va_list ap);
char *str_format_va(const char *fmt, va_list ap);
char *str_format(const char *fmt, ...);
bool str_starts_with(const char *s, const char *start);
bool str_ends_with(const char *s, const char *end);
char *str_expand_tilde(const char *path);
char *str_compress_tilde(const char *path);
char *str_read_file(const char *pathname);

#endif
#ifdef CSTR_IMPLEMENTATION

int str_get_format_size(const char *fmt, va_list ap)
{
  va_list ap1;
  va_copy(ap1, ap);
  int size = vsnprintf(NULL, 0, fmt, ap1);
  va_end(ap1);

  return size;
}

char *str_format_va(const char *fmt, va_list ap)
{
  int size = str_get_format_size(fmt, ap) + 1;

  va_list ap1;
  va_copy(ap1, ap);
  char *str = malloc(sizeof(char) * size);
  vsnprintf(str, size, fmt, ap1);
  va_end(ap1);

  return str;
}

char *str_format(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  char *str = str_format_va(fmt, ap);
  va_end(ap);

  return str;
}

bool str_starts_with(const char *s, const char *start)
{
  return !strncmp(s, start, strlen(start));
}

bool str_ends_with(const char *s, const char *end)
{
  if (strlen(end) > strlen(s)) {
    return false;
  }

  return !strcmp(s + strlen(s) - strlen(end), end);
}

char *str_expand_tilde(const char *path)
{
  if (str_starts_with(path, "~")) {
    return str_format("%s%s", getenv("HOME"), path + 1);
  }

  return str_format("%s", path);
}

char *str_compress_tilde(const char *path)
{
  const char *home = getenv("HOME");

  if (str_starts_with(path, home)) {
    return str_format("~%s", path + strlen(home));
  }

  return str_format("%s", path);
}

int str_get_file_size(FILE *fp)
{
  int curr = ftell(fp);
  fseek(fp, 0, SEEK_END);

  int size = ftell(fp);
  fseek(fp, curr, SEEK_SET);

  return size;
}

char *str_read_file(const char *pathname)
{
  FILE *fp = fopen(pathname, "r");

  if (fp == NULL) {
    return NULL;
  }

  int file_size = str_get_file_size(fp);

  char *content = malloc(file_size + 1);
  fread(content, 1, file_size, fp);
  content[file_size] = '\0';

  fclose(fp);
  return content;
}

#endif
