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
int str_count_matches(const char *haystack, const char *needle);
int str_array_len(char **array);
char **str_split(const char *s, const char *delim);
void str_free_array(char **array);
char *str_replace(const char *s, const char *old, const char *new);
int str_array_len(char **array);

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

void str_free_array(char **array)
{
  for (char **s = array; *s; s++) {
    free(*s);
  }

  free(array);
}

int str_array_len(char **array)
{
  int len = 0;

  while (array[len]) {
    len++;
  }

  return len;
}

int str_count_matches(const char *haystack, const char *needle)
{
  int n = 0;

  for (const char *p = haystack; (p = strstr(p, needle)); p += strlen(needle)) {
    n++;
  }

  return n;
}

char **str_split(const char *s, const char *delim)
{
  char **array = malloc(sizeof(char *) * (str_count_matches(s, delim) + 2));

  const char *start = s;
  const char *end;
  int i = 0;

  while ((end = strstr(start, delim))) {
    array[i++] = strndup(start, end - start);
    start = end += strlen(delim);
  }

  array[i++] = strdup(start);
  array[i++] = NULL;

  return array;
}

char *str_replace(const char *s, const char *old, const char *new)
{
  if (strlen(old) == 0) {
    return strdup(s);
  }

  int new_size = strlen(s) + (str_count_matches(s, old) * (strlen(new) - strlen(old)));
  char *res = malloc(sizeof(char) * (new_size + 1));

  char *dst = res;
  const char *src = s;

  while (*src) {
    if (!strncmp(src, old, strlen(old))) {
      strcpy(dst, new);
      dst += strlen(new);
      src += strlen(old);
    } else {
      *dst++ = *src++;
    }
  }

  *dst = '\0';
  return res;
}

#endif
