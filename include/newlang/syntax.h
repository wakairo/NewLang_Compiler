#ifndef NEWLANG_SYNTAX_H
#define NEWLANG_SYNTAX_H

#include "newlang/source.h"

typedef struct NLSyntaxTree NLSyntaxTree;
typedef struct NLSyntaxNode NLSyntaxNode;

typedef enum {
    NL_SYNTAX_TYPE_NAME,
    NL_SYNTAX_TYPE_PTR,
    NL_SYNTAX_TYPE_REF,
    NL_SYNTAX_EXPR_NAME,
    NL_SYNTAX_EXPR_CALL,
    NL_SYNTAX_BINDING,
    NL_SYNTAX_LOAN
} NLSyntaxKind;

/* Requested source spelling only: not checked access permission/authority. */
typedef enum {
    NL_ACCESS_READ,
    NL_ACCESS_WRITE
} NLAccessSyntax;

/* Immutable public inspection contract, not the owning/private node layout.
 * Read only the union member selected by kind. All child/view pointers borrow
 * the tree until destroy. Names are source spans; no strings or semantic IDs.
 * Optional loan stability is NULL. Calls retain arguments in source order. */
typedef struct {
    NLSyntaxKind kind;
    NLSourceSpan span;
    union {
        NLSourceSpan name; /* TYPE_NAME / EXPR_NAME. */
        struct {
            const NLSyntaxNode *target;
        } ptr_type;
        struct {
            NLAccessSyntax access;
            bool is_exclusive;
            const NLSyntaxNode *target;
        } ref_type;
        struct {
            NLSourceSpan callee;
            const NLSyntaxNode *arguments;
            size_t argument_count;
        } call;
        struct {
            NLSourceSpan name;
            const NLSyntaxNode *initializer;
        } binding;
        struct {
            NLAccessSyntax access;
            bool is_exclusive;
            const NLSyntaxNode *source;
            const NLSyntaxNode *stability;
            NLSourceSpan binding;
            NLSourceSpan body_open;
            NLSourceSpan body_interior;
            NLSourceSpan body_close;
        } loan;
    } data;
} NLSyntaxView;

/* Trees own every node, borrow their source, and have one root. The source must
 * outlive tree use/text access. Parser reuse/destruction does not invalidate a
 * returned tree. Destroy consumes tree/nodes once, never source; NULL allowed.
 * Cleanup is iterative even for nested syntax. */
void nl_syntax_tree_destroy(NLSyntaxTree *tree);
const NLSource *nl_syntax_tree_source(const NLSyntaxTree *tree);
const NLSyntaxNode *nl_syntax_tree_root(const NLSyntaxTree *tree);
const NLSyntaxView *nl_syntax_node_view(const NLSyntaxNode *node);

/* Walk call arguments in source order; NULL terminates. Non-argument nodes
 * have no next argument. All getters return NULL for a NULL input. */
const NLSyntaxNode *nl_syntax_next_argument(const NLSyntaxNode *node);

#endif
