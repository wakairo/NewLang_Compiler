#ifndef NEWLANG_SYNTAX_CHECK_H
#define NEWLANG_SYNTAX_CHECK_H

#include "newlang/syntax.h"
#include "test.h"

#include <string.h>

static bool test_name(const NLSource *source, NLSourceSpan span,
                      const char *name)
{
    NLSourceView view;
    CHECK(nl_source_view(source, span, &view));
    CHECK(view.length == strlen(name));
    CHECK(memcmp(view.bytes, name, view.length) == 0);
    return true;
}

static bool test_contains(NLSourceSpan parent, NLSourceSpan child)
{
    CHECK(parent.start_byte <= child.start_byte);
    CHECK(child.start_byte <= child.end_byte &&
          child.end_byte <= parent.end_byte);
    return true;
}

static bool test_node(const NLSource *source, const NLSyntaxNode *node,
                      NLSourceSpan parent)
{
    const NLSyntaxView *const view = nl_syntax_node_view(node);
    CHECK(view != NULL && nl_source_span_valid(source, view->span));
    CHECK(test_contains(parent, view->span));
    switch (view->kind) {
    case NL_SYNTAX_TYPE_NAME:
    case NL_SYNTAX_EXPR_NAME:
        CHECK(test_contains(view->span, view->data.name));
        CHECK(view->data.name.start_byte < view->data.name.end_byte);
        break;
    case NL_SYNTAX_TYPE_PTR:
        CHECK(test_node(source, view->data.ptr_type.target, view->span));
        break;
    case NL_SYNTAX_TYPE_REF:
        CHECK(view->data.ref_type.access == NL_ACCESS_READ ||
              view->data.ref_type.access == NL_ACCESS_WRITE);
        CHECK(test_node(source, view->data.ref_type.target, view->span));
        break;
    case NL_SYNTAX_EXPR_CALL: {
        CHECK(test_contains(view->span, view->data.call.callee));
        size_t count = 0;
        size_t previous_end = view->data.call.callee.end_byte;
        for (const NLSyntaxNode *arg = view->data.call.arguments; arg != NULL;
             arg = nl_syntax_next_argument(arg)) {
            const NLSyntaxView *const argument = nl_syntax_node_view(arg);
            CHECK(argument->span.start_byte >= previous_end);
            CHECK(test_node(source, arg, view->span));
            previous_end = argument->span.end_byte;
            ++count;
        }
        CHECK(count == view->data.call.argument_count);
        break;
    }
    case NL_SYNTAX_BINDING:
        CHECK(test_contains(view->span, view->data.binding.name));
        CHECK(test_node(source, view->data.binding.initializer, view->span));
        break;
    case NL_SYNTAX_LOAN:
        CHECK(test_node(source, view->data.loan.source, view->span));
        if (view->data.loan.stability != NULL) {
            CHECK(test_node(source, view->data.loan.stability, view->span));
        }
        CHECK(test_contains(view->span, view->data.loan.binding));
        CHECK(test_contains(view->span, view->data.loan.body_open));
        CHECK(test_contains(view->span, view->data.loan.body_interior));
        CHECK(test_contains(view->span, view->data.loan.body_close));
        CHECK(view->data.loan.body_open.end_byte ==
              view->data.loan.body_interior.start_byte);
        CHECK(view->data.loan.body_interior.end_byte ==
              view->data.loan.body_close.start_byte);
        CHECK(view->span.end_byte == view->data.loan.body_close.end_byte);
        CHECK(test_name(source, view->data.loan.body_open, "{"));
        CHECK(test_name(source, view->data.loan.body_close, "}"));
        break;
    default:
        CHECK(false);
    }
    return true;
}

static bool test_tree(const NLSyntaxTree *tree)
{
    const NLSource *const source = nl_syntax_tree_source(tree);
    CHECK(source != NULL);
    CHECK(test_node(source, nl_syntax_tree_root(tree),
                    (NLSourceSpan){0, nl_source_length(source)}));
    return true;
}

static bool test_equivalent(const NLSyntaxNode *left, const NLSyntaxNode *right)
{
    if (left == NULL || right == NULL) {
        CHECK(left == NULL && right == NULL);
        return true;
    }
    const NLSyntaxView *const a = nl_syntax_node_view(left);
    const NLSyntaxView *const b = nl_syntax_node_view(right);
    CHECK(a->kind == b->kind && a->span.start_byte == b->span.start_byte &&
          a->span.end_byte == b->span.end_byte);
    switch (a->kind) {
    case NL_SYNTAX_TYPE_NAME:
    case NL_SYNTAX_EXPR_NAME:
        CHECK(a->data.name.start_byte == b->data.name.start_byte &&
              a->data.name.end_byte == b->data.name.end_byte);
        break;
    case NL_SYNTAX_TYPE_PTR:
        CHECK(
            test_equivalent(a->data.ptr_type.target, b->data.ptr_type.target));
        break;
    case NL_SYNTAX_TYPE_REF:
        CHECK(a->data.ref_type.access == b->data.ref_type.access &&
              a->data.ref_type.is_exclusive == b->data.ref_type.is_exclusive);
        CHECK(
            test_equivalent(a->data.ref_type.target, b->data.ref_type.target));
        break;
    case NL_SYNTAX_EXPR_CALL: {
        CHECK(a->data.call.callee.start_byte ==
                  b->data.call.callee.start_byte &&
              a->data.call.callee.end_byte == b->data.call.callee.end_byte);
        CHECK(a->data.call.argument_count == b->data.call.argument_count);
        const NLSyntaxNode *x = a->data.call.arguments;
        const NLSyntaxNode *y = b->data.call.arguments;
        while (x != NULL || y != NULL) {
            CHECK(test_equivalent(x, y));
            x = nl_syntax_next_argument(x);
            y = nl_syntax_next_argument(y);
        }
        break;
    }
    case NL_SYNTAX_BINDING:
        CHECK(a->data.binding.name.start_byte ==
                  b->data.binding.name.start_byte &&
              a->data.binding.name.end_byte == b->data.binding.name.end_byte);
        CHECK(test_equivalent(a->data.binding.initializer,
                              b->data.binding.initializer));
        break;
    case NL_SYNTAX_LOAN:
        CHECK(a->data.loan.access == b->data.loan.access &&
              a->data.loan.is_exclusive == b->data.loan.is_exclusive);
        CHECK(test_equivalent(a->data.loan.source, b->data.loan.source));
        CHECK(test_equivalent(a->data.loan.stability, b->data.loan.stability));
        CHECK(a->data.loan.binding.start_byte ==
                  b->data.loan.binding.start_byte &&
              a->data.loan.binding.end_byte == b->data.loan.binding.end_byte);
        CHECK(a->data.loan.body_open.start_byte ==
                  b->data.loan.body_open.start_byte &&
              a->data.loan.body_open.end_byte ==
                  b->data.loan.body_open.end_byte);
        CHECK(a->data.loan.body_interior.start_byte ==
                  b->data.loan.body_interior.start_byte &&
              a->data.loan.body_interior.end_byte ==
                  b->data.loan.body_interior.end_byte);
        CHECK(a->data.loan.body_close.start_byte ==
                  b->data.loan.body_close.start_byte &&
              a->data.loan.body_close.end_byte ==
                  b->data.loan.body_close.end_byte);
        break;
    default:
        CHECK(false);
    }
    return true;
}

#endif
