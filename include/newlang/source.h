#ifndef NEWLANG_SOURCE_H
#define NEWLANG_SOURCE_H

#include <stdbool.h>
#include <stddef.h>

/* Owns immutable logical bytes and a copied NUL-terminated display name.
 * Contents are always length-based; ingestion accepts/preserves every byte,
 * without asserting language validity. No registry: object + span is identity.
 * Opaque so callers cannot mutate contents or duplicate owning fields. */
typedef struct NLSource NLSource;

typedef struct {
    size_t start_byte;
    size_t end_byte;
} NLSourceSpan;

typedef struct {
    const unsigned char *bytes;
    size_t length;
} NLSourceView;

typedef enum {
    NL_SOURCE_OK,
    NL_SOURCE_INVALID_ARGUMENT,
    NL_SOURCE_TOO_LARGE,
    NL_SOURCE_OUT_OF_MEMORY,
    NL_SOURCE_IO_ERROR
} NLSourceStatus;

/* out_source must point to an initialized NULL owner slot. All inputs are
 * borrowed for the call; bytes may be NULL only when length == 0. Success
 * returns a new owner; later input/name changes cannot affect it. Failure
 * leaves the slot untouched, retains no inputs, and cleans up partial state.
 * A nonempty destination is rejected (never overwritten). Lengths greater than
 * PTRDIFF_MAX are unsupported storage sizes. OOM is a host failure, not a
 * lexical error. No allocation failure is fatal to the process. */
NLSourceStatus nl_source_create(const void *bytes, size_t length,
                                const char *display_name,
                                NLSource **out_source);

/* path is a borrowed NUL-terminated filesystem name, copied as display name.
 * Reads in binary mode, without size/seek assumptions or text normalization.
 * Same output/ownership/failure contract as create. Open/read/close failures
 * return IO_ERROR; allocation failures return OUT_OF_MEMORY. No partial source
 * is returned. This is a synchronous file read, not an atomic file snapshot. */
NLSourceStatus nl_source_load(const char *path, NLSource **out_source);

/* Consumes the source and its bytes/name; NULL is allowed. Invalidate all views
 * and lexer borrows first. Destroy exactly once for each successful creation.
 */
void nl_source_destroy(NLSource *source);

/* source must be live/non-NULL. Name and views are borrowed until destroy;
 * neither may be freed or modified. Empty contents may have a NULL pointer. */
size_t nl_source_length(const NLSource *source);
const char *nl_source_name(const NLSource *source);

/* Valid iff start <= end <= length; empty/end-of-input spans are valid.
 * NULL source is invalid. View failure leaves output untouched. An empty view
 * may contain NULL or a one-past pointer and must never be dereferenced. */
bool nl_source_span_valid(const NLSource *source, NLSourceSpan span);
bool nl_source_view(const NLSource *source, NLSourceSpan span,
                    NLSourceView *out_view);

#endif
