#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#include "setup.h"
#include "util.h"

#define DIR_SEPARATOR   "/"

void
lr_out_of_memory()
{
    fprintf(stderr, "Out of memory\n");
    abort();
    exit(1);
}

void *
lr_malloc(size_t len)
{
    void *m = malloc(len);
    if (!m) lr_out_of_memory();
    return m;
}

void *
lr_malloc0(size_t len)
{
    void *m = calloc(1, len);
    if (!m) lr_out_of_memory();
    return m;
}

void *
lr_realloc(void *ptr, size_t len)
{
    void *m = realloc(ptr, len);
    if (!m && len) lr_out_of_memory();
    return m;
}

void
lr_free(void *m)
{
    if (m) free(m);
}

char *
lr_strdup(const char *str)
{
    char *new;
    if (!str)
        return NULL;
    new = strdup(str);
    if (!new) lr_out_of_memory();
    return new;
}

char *
lr_strconcat(const char *str, ...)
{
    va_list arg;
    char *chunk, *res;
    size_t offset, total_len;

    if (!str)
        return NULL;

    offset = strlen(str);
    total_len = offset;

    va_start(arg, str);
    while ((chunk = va_arg(arg, char *)))
        total_len += strlen(chunk);
    va_end(arg);

    res = lr_malloc(total_len + 1);

    strcpy(res, str);
    va_start(arg, str);
    while ((chunk = va_arg(arg, char *))) {
        strcpy(res + offset, chunk);
        offset += strlen(chunk);
    }
    va_end(arg);

    return res;
}

int
lr_gettmpfile()
{
    char template[] = "/tmp/librepo-tmp-XXXXXX";
    int fd = mkstemp(template);
    if (fd < 0) {
        perror("Cannot create temporary file - mkstemp");
        exit(1);
    }
    unlink(template);
    return fd;
}

int
lr_ends_with(const char *str, const char *suffix)
{
    int str_len;
    int suffix_len;

    assert(str);
    assert(suffix);

    str_len = strlen(str);
    suffix_len = strlen(suffix);

    if (str_len < suffix_len)
        return 0;

    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

char *
lr_pathconcat(const char *first, ...)
{
    va_list args;
    const char *next;
    char *separator = DIR_SEPARATOR;
    char *chunk, *res = NULL;
    size_t separator_len = strlen(DIR_SEPARATOR);
    size_t total_len;  // Maximal len of result
    size_t offset = 0;
    int is_first = 1;
    int previous_was_empty = 0; // If last chunk was "" then separator will be
                                // appended to the result

    if (!first)
        return NULL;

    total_len = strlen(first);

    va_start(args, first);
    while ((chunk = va_arg(args, char *)))
        total_len += (strlen(chunk) + separator_len);
    va_end(args);

    if (total_len == 0)
        return lr_strdup("");

    res = lr_malloc(total_len + separator_len + 1);

    next = first;
    va_start(args, first);
    while (1) {
        const char *current, *start, *end;
        size_t current_len;

        if (next) {
            current = next;
            next = va_arg(args, char *);
        } else
            break;

        current_len = strlen(current);

        if (!current_len) {
            previous_was_empty = 1;
            continue;   /* Skip empty element */
        } else
            previous_was_empty = 0;

        start = current;
        end = start + current_len;

        /* Skip leading separators - except first element */
        if (separator_len && is_first == 0) {
            while (!strncmp(start, separator, separator_len))
                start += separator_len;
        }

        /* Skip trailing separators */
        if (separator_len) {
            while (start + separator_len <= end &&
                   !strncmp(end-separator_len, separator, separator_len))
                end -= separator_len;
        }

        if (start >= end) {
            /* Element is filled only by separators */
            if (is_first)
                is_first = 0;
            continue;
        }

        /* Prepend separator - except first element */
        if (is_first == 0) {
            strncpy(res + offset, separator, separator_len);
            offset += separator_len;
        } else
            is_first = 0;

        strncpy(res + offset, start, end - start);
        offset += end - start;
    }
    va_end(args);

    DEBUGASSERT(offset <= total_len);

    if (offset == 0) {
        lr_free(res);
        return lr_strdup(first);
    }

    /* If last element was emtpy string, append separator to the end */
    if (previous_was_empty && is_first == 0) {
        strncpy(res + offset, separator, separator_len);
        offset += separator_len;
    }

    DEBUGASSERT(offset <= total_len);

    res[offset] = '\0';

    return res;
}
