#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PARSER_H_

typedef struct {
  size_t count;
  const char *data;
} String_View;

#define DA_INIT_CAP 256

// Append an item to a dynamic array
#define da_append(da, item)                                                    \
  do {                                                                         \
    if ((da)->count >= (da)->capacity) {                                       \
      (da)->capacity = (da)->capacity == 0 ? DA_INIT_CAP : (da)->capacity * 2; \
      (da)->items =                                                            \
          realloc((da)->items, (da)->capacity * sizeof(*(da)->items));         \
    }                                                                          \
                                                                               \
    (da)->items[(da)->count++] = (item);                                       \
  } while (0)

String_View sv_chop_by_delim(String_View *sv, char delim);

String_View sv_chop_by_delim(String_View *sv, char delim) {
  size_t i = 0;
  while (i < sv->count && sv->data[i] != delim) {
    i += 1;
  }

  String_View result = {
      .count = i,
      .data = sv->data,
  };

  if (i < sv->count) {
    sv->count -= i + 1;
    sv->data += i + 1;
  } else {
    sv->count -= i;
    sv->data += i;
  }

  return result;
}

typedef struct {
  char *items;
  size_t count;
  size_t capacity;
} String_Builder;

bool read_entire_file(const char *path, String_Builder *sb);
bool read_entire_file(const char *path, String_Builder *sb) {
  bool result = true;
  FILE *f = fopen(path, "rb");
  if (f == NULL) {
    if (!result)
      fprintf(stderr, "could not read file %s: %s", path, strerror(errno));
    if (f)
      fclose(f);
    return result;
  }
  if (fseek(f, 0, SEEK_END) < 0) {
    if (!result)
      fprintf(stderr, "could not read file %s: %s", path, strerror(errno));
    if (f)
      fclose(f);
    return result;
  }
  long m = ftell(f);
  if (m < 0) {
    if (!result)
      fprintf(stderr, "could not read file %s: %s", path, strerror(errno));
    if (f)
      fclose(f);
    return result;
  }
  if (fseek(f, 0, SEEK_SET) < 0) {
    if (!result)
      fprintf(stderr, "could not read file %s: %s", path, strerror(errno));
    if (f)
      fclose(f);
    return result;
  }

  size_t new_count = sb->count + m;
  if (new_count > sb->capacity) {
    sb->items = realloc(sb->items, new_count);
    sb->capacity = new_count;
  }

  fread(sb->items + sb->count, m, 1, f);
  if (ferror(f)) {
    if (!result)
      fprintf(stderr, "could not read file %s: %s", path, strerror(errno));
    if (f)
      fclose(f);
    return result;
  }
  sb->count = new_count;
}
