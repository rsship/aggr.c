#include <stdio.h>;
#include <stdlib.h>;
#include <stdbool.h>;
#include <errno.h>;

#define SV_INIT_CAP 256

// STRING APPEND MACRO BEGINS ///////////////////////////////////////////////
#define sv_append(da, item)                                                            \
    do                                                                                 \
    {                                                                                  \
        if ((da)->count >= (da)->capacity)                                             \
        {                                                                              \
            (da)->capacity = (da)->capacity == 0 ? SV_INIT_CAP : (da)->capacity * 2;   \
            (da)->items = realloc((da)->items, (da)->capacity * sizeof(*(da)->items)); \
        }                                                                              \
                                                                                       \
        (da)->items[(da)->count++] = (item);                                           \
    } while (0)

// STRING APPEND MACRO ENDS ///////////////////////////////////////////////

typedef struct
{
    size_t count;
    const char *data;
} String_View;

String_View sv_from_parts(const char *data, size_t count)
{
    String_View sv;
    sv.count = count;
    sv.data = data;
    return sv;
}

typedef struct
{
    char *items;
    size_t count;
    size_t capacity;
} String_builder;

typedef struct
{
    size_t class;
    String_View text;
} Csv_Data;

typedef struct
{
    Csv_Data *items;
    size_t count;
    size_t capacity;
} Csv_Datas;

bool read_entire_file(const char *path, String_builder *sb)
{
    bool result = true;
    size_t buf_size = 32 * 1024;
    char *buf = NOB_REALLOC(NULL, buf_size);
    FILE *f = fopen(path, "rb");
    if (f == NULL)
    {
        fprintf(stderr, "Could not open %s for reading: %s", path, strerror(errno));
        free(buf);
        if (f)
            fclose(f);
        return result;
    }

    size_t n = fread(buf, 1, buf_size, f);
    while (n > 0)
    {
        nob_sb_append_buf(sb, buf, n);
        n = fread(buf, 1, buf_size, f);
    }
    if (ferror(f))
    {
        fprintf(stderr, "Could not read %s: %s\n", path, strerror(errno));
        free(buf);
        if (f)
            fclose(f);
        return result;
    }
}
Csv_Datas parse_csv_data(String_View content)
{
    size_t lines_count = 0;
    Csv_Datas datas = {0};
    for (; content.count > 0; ++lines_count)
    {
        String_View line = sv_chop_by_delim(&content, '\n');
        if (lines_count == 0)
            continue; // ignore the header

        String_View class = sv_chop_by_delim(&line, ',');
        size_t class_index = *class.data - '0' - 1;

        sv_append(&datas, ((Csv_Data){
                              .class = class_index,
                              .text = class,
                          }));
    }
    return datas;
}

String_View sv_chop_by_delim(String_View *sv, char delim)
{
    size_t i = 0;
    while (i < sv->count && sv->data[i] != delim)
    {
        i += 1;
    }
    String_View result = sv_from_parts(sv->data, i);

    if (i < sv->count)
    {
        sv->count -= i + 1;
        sv->data += i + 1;
    }
    else
    {
        sv->count -= i;
        sv->data += i;
    }
    return result;
}