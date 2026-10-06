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
    NL_SYNTAX_LOAN,
    NL_SYNTAX_BLOCK,
    NL_SYNTAX_STATEMENT,
    NL_SYNTAX_RECEIVER,
    NL_SYNTAX_FIELD,
    NL_SYNTAX_MULTI_BINDING,
    NL_SYNTAX_AGGREGATE_BINDING,
    NL_SYNTAX_AGGREGATE,
    NL_SYNTAX_SUM_CONSTRUCTOR,
    NL_SYNTAX_MATCH,
    NL_SYNTAX_IF,
    NL_SYNTAX_MATCH_ARM,
    NL_SYNTAX_FUNCTION_UNIT,
    NL_SYNTAX_FUNCTION,
    NL_SYNTAX_PARAMETER,
    NL_SYNTAX_RETURN /* dedicated block item; data.statement.expression */
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
        struct {
            const NLSyntaxNode *declarations;
            size_t count;
        } function_unit;
        struct {
            NLSourceSpan name;
            const NLSyntaxNode *parameters, *result, *body;
            size_t count;
        } function;
        struct {
            NLSourceSpan name;
            const NLSyntaxNode *type;
        } parameter;
        NLSourceSpan name; /* TYPE_NAME / EXPR_NAME / RECEIVER. */
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
            const NLSyntaxNode *items, *tail;
            size_t item_count;
        } block;
        struct {
            const NLSyntaxNode *expression;
        } statement;
        struct {
            const NLSyntaxNode *receivers, *initializer;
            size_t count;
        } multi_binding;
        struct {
            NLSourceSpan type_name;
            const NLSyntaxNode *fields, *initializer;
            size_t count;
        } aggregate;
        struct {
            NLSourceSpan qualifier, variant;
            const NLSyntaxNode *arguments;
            size_t argument_count;
            bool parentheses;
        } constructor;
        struct {
            const NLSyntaxNode *condition, *then_block, *else_block;
        } conditional;
        struct {
            const NLSyntaxNode *scrutinee, *arms;
            size_t arm_count;
        } match;
        struct {
            NLSourceSpan variant, binding;
            bool payload, wildcard;
            const NLSyntaxNode *body;
        } arm;
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

/* Walk call arguments, block items, receivers, aggregate fields, function
 * declarations or parameters in source
 * order; NULL terminates. Non-argument nodes have no next argument. All getters
 * return NULL for a NULL input. */
const NLSyntaxNode *nl_syntax_next_argument(const NLSyntaxNode *node);

#endif
