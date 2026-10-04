#include "newlang/source.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct NLSource {
    unsigned char *bytes;
    size_t length;
    char *display_name;
};

/* Takes ownership of bytes only on success. Caller cleans it up on failure. */
static NLSourceStatus finish_source(unsigned char *bytes, size_t length,
                                    const char *name, NLSource **out_source)
{
    const size_t name_length = strlen(name);
    if (name_length == SIZE_MAX) {
        return NL_SOURCE_TOO_LARGE;
    }
    NLSource *const source = malloc(sizeof(*source));
    if (source == NULL) {
        return NL_SOURCE_OUT_OF_MEMORY;
    }
    char *const name_copy = malloc(name_length + 1);
    if (name_copy == NULL) {
        free(source);
        return NL_SOURCE_OUT_OF_MEMORY;
    }
    memcpy(name_copy, name, name_length + 1);
    *source = (NLSource){bytes, length, name_copy};
    *out_source = source;
    return NL_SOURCE_OK;
}

NLSourceStatus nl_source_create(const void *bytes, size_t length,
                                const char *display_name, NLSource **out_source)
{
    if (out_source == NULL || *out_source != NULL || display_name == NULL ||
        (bytes == NULL && length != 0)) {
        return NL_SOURCE_INVALID_ARGUMENT;
    }
    if (length > (size_t)PTRDIFF_MAX) {
        return NL_SOURCE_TOO_LARGE;
    }
    unsigned char *copy = NULL;
    if (length != 0) {
        copy = malloc(length);
        if (copy == NULL) {
            return NL_SOURCE_OUT_OF_MEMORY;
        }
        memcpy(copy, bytes, length);
    }
    const NLSourceStatus status =
        finish_source(copy, length, display_name, out_source);
    if (status != NL_SOURCE_OK) {
        free(copy);
    }
    return status;
}

NLSourceStatus nl_source_load(const char *path, NLSource **out_source)
{
    if (out_source == NULL || *out_source != NULL || path == NULL) {
        return NL_SOURCE_INVALID_ARGUMENT;
    }
    FILE *const file = fopen(path, "rb");
    if (file == NULL) {
        return NL_SOURCE_IO_ERROR;
    }
    unsigned char *bytes = NULL;
    size_t length = 0;
    size_t capacity = 0;
    NLSourceStatus status = NL_SOURCE_OK;
    unsigned char block[4096];
    for (;;) {
        const size_t count = fread(block, 1, sizeof(block), file);
        if (count > (size_t)PTRDIFF_MAX - length) {
            status = NL_SOURCE_TOO_LARGE;
            break;
        }
        const size_t required = length + count;
        if (required > capacity) {
            size_t next_capacity = capacity == 0 ? sizeof(block) : capacity;
            while (next_capacity < required) {
                if (next_capacity > (size_t)PTRDIFF_MAX / 2) {
                    next_capacity = required;
                    break;
                }
                next_capacity *= 2;
            }
            unsigned char *const grown = realloc(bytes, next_capacity);
            if (grown == NULL) {
                status = NL_SOURCE_OUT_OF_MEMORY;
                break;
            }
            bytes = grown;
            capacity = next_capacity;
        }
        if (count != 0) {
            memcpy(bytes + length, block, count);
            length = required;
        }
        if (ferror(file)) {
            status = NL_SOURCE_IO_ERROR;
            break;
        }
        if (feof(file)) {
            break;
        }
    }
    if (fclose(file) != 0 && status == NL_SOURCE_OK) {
        status = NL_SOURCE_IO_ERROR;
    }
    if (status == NL_SOURCE_OK) {
        status = finish_source(bytes, length, path, out_source);
    }
    if (status != NL_SOURCE_OK) {
        free(bytes);
    }
    return status;
}

void nl_source_destroy(NLSource *source)
{
    if (source != NULL) {
        free(source->bytes);
        free(source->display_name);
        free(source);
    }
}

size_t nl_source_length(const NLSource *source)
{
    assert(source != NULL);
    return source->length;
}

const char *nl_source_name(const NLSource *source)
{
    assert(source != NULL);
    return source->display_name;
}

bool nl_source_span_valid(const NLSource *source, NLSourceSpan span)
{
    return source != NULL && span.start_byte <= span.end_byte &&
           span.end_byte <= source->length;
}

bool nl_source_view(const NLSource *source, NLSourceSpan span,
                    NLSourceView *out_view)
{
    if (out_view == NULL || !nl_source_span_valid(source, span)) {
        return false;
    }
    const unsigned char *const bytes =
        source->bytes == NULL ? NULL : source->bytes + span.start_byte;
    *out_view = (NLSourceView){bytes, span.end_byte - span.start_byte};
    return true;
}
