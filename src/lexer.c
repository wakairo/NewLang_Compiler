#include "newlang/lexer.h"

static bool word_start(unsigned char byte)
{
    return (byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z') ||
           byte == '_';
}

static bool digit(unsigned char byte)
{
    return byte >= '0' && byte <= '9';
}

static bool punctuation(unsigned char byte)
{
    switch (byte) {
    case '(':
    case ')':
    case '{':
    case '}':
    case '[':
    case ']':
    case ',':
    case ':':
    case ';':
    case '.':
    case '+':
    case '-':
    case '*':
    case '/':
    case '%':
    case '<':
    case '>':
    case '=':
    case '!':
    case '&':
    case '|':
    case '?':
        return true;
    default:
        return false;
    }
}

bool nl_lexer_init(NLLexer *lexer, const NLSource *source)
{
    if (lexer == NULL) {
        return false;
    }
    *lexer = (NLLexer){source, 0};
    return source != NULL;
}

NLLexResult nl_lexer_next(NLLexer *lexer, NLToken *out_token)
{
    if (lexer == NULL || lexer->source == NULL || out_token == NULL) {
        return NL_LEX_INTERNAL_ERROR;
    }
    const size_t length = nl_source_length(lexer->source);
    if (lexer->next_byte > length) {
        return NL_LEX_INTERNAL_ERROR;
    }
    NLSourceView view;
    if (!nl_source_view(lexer->source, (NLSourceSpan){0, length}, &view)) {
        return NL_LEX_INTERNAL_ERROR;
    }
    const unsigned char *const bytes = view.bytes;
    size_t start = lexer->next_byte;
    while (start < length) {
        const unsigned char byte = bytes[start];
        if (byte == ' ' || byte == '\t' || byte == '\n') {
            ++start;
        } else if (byte == '\r' && length - start >= 2 &&
                   bytes[start + 1] == '\n') {
            start += 2;
        } else {
            break;
        }
    }
    if (start == length) {
        lexer->next_byte = length;
        *out_token = (NLToken){NL_TOKEN_EOF, {length, length}};
        return NL_LEX_EOF;
    }
    size_t end = start + 1;
    NLTokenKind kind;
    if (word_start(bytes[start])) {
        kind = NL_TOKEN_WORD;
        while (end < length && (word_start(bytes[end]) || digit(bytes[end]))) {
            ++end;
        }
    } else if (digit(bytes[start])) {
        kind = NL_TOKEN_DIGITS;
        while (end < length && digit(bytes[end])) {
            ++end;
        }
    } else if (bytes[start] == '/' && length - start >= 2 &&
               (bytes[start + 1] == '/' || bytes[start + 1] == '*')) {
        kind = NL_TOKEN_UNSUPPORTED;
        ++end;
    } else if (punctuation(bytes[start])) {
        kind = NL_TOKEN_PUNCTUATION;
    } else {
        kind = NL_TOKEN_UNSUPPORTED;
    }
    *out_token = (NLToken){kind, {start, end}};
    if (kind == NL_TOKEN_UNSUPPORTED) {
        lexer->next_byte = start;
        return NL_LEX_UNSUPPORTED;
    }
    lexer->next_byte = end;
    return NL_LEX_TOKEN;
}
