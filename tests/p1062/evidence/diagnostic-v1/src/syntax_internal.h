#ifndef NEWLANG_SYNTAX_INTERNAL_H
#define NEWLANG_SYNTAX_INTERNAL_H

#include "newlang/syntax.h"

/* Private construction state shared only by parser/syntax implementations. */
struct NLSyntaxNode {
    NLSyntaxView view;
    struct NLSyntaxNode *owned_next;
    struct NLSyntaxNode *argument_next;
};

struct NLSyntaxTree {
    const NLSource *source;
    NLSyntaxNode *root;
    NLSyntaxNode *nodes;
    size_t node_count;
};

NLSyntaxTree *nl_syntax_tree_create(const NLSource *source);
NLSyntaxNode *nl_syntax_node_create(NLSyntaxTree *tree, NLSyntaxKind kind,
                                    NLSourceSpan span);

#endif
