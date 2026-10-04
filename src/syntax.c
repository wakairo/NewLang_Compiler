#include "syntax_internal.h"

#include <stdlib.h>

NLSyntaxTree *nl_syntax_tree_create(const NLSource *source)
{
    NLSyntaxTree *const tree = malloc(sizeof(*tree));
    if (tree != NULL) {
        *tree = (NLSyntaxTree){.source = source};
    }
    return tree;
}

NLSyntaxNode *nl_syntax_node_create(NLSyntaxTree *tree, NLSyntaxKind kind,
                                    NLSourceSpan span)
{
    NLSyntaxNode *const node = malloc(sizeof(*node));
    if (node != NULL) {
        *node = (NLSyntaxNode){.view = {.kind = kind, .span = span},
                               .owned_next = tree->nodes};
        tree->nodes = node;
        ++tree->node_count; /* Bounded by parser's explicit node budget. */
    }
    return node;
}

void nl_syntax_tree_destroy(NLSyntaxTree *tree)
{
    if (tree != NULL) {
        NLSyntaxNode *node = tree->nodes;
        while (node != NULL) {
            NLSyntaxNode *const next = node->owned_next;
            free(node);
            node = next;
        }
        free(tree);
    }
}

const NLSource *nl_syntax_tree_source(const NLSyntaxTree *tree)
{
    return tree == NULL ? NULL : tree->source;
}

const NLSyntaxNode *nl_syntax_tree_root(const NLSyntaxTree *tree)
{
    return tree == NULL ? NULL : tree->root;
}

const NLSyntaxView *nl_syntax_node_view(const NLSyntaxNode *node)
{
    return node == NULL ? NULL : &node->view;
}

const NLSyntaxNode *nl_syntax_next_argument(const NLSyntaxNode *node)
{
    return node == NULL ? NULL : node->argument_next;
}
