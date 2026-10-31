#include "../shared.h"

static bool probe(const char *path)
{
    NLSemanticContext *base = NULL;
    TestNode a = {0}, b = {0};
    CHECK(registry(path, &base) && checked(base, &a) && checked(base, &b));
    char *baseline = NULL;
    size_t bytes = 0;
    CHECK(nl_checked_c_node(a.entry, &baseline, &bytes) == NL_NODE_C_OK &&
          bytes);
    const NLSource *spelling = a.entry->source;
    a.entry->source = NULL;
    CHECK(!nl_h_custody_validate(a.entry) && emit(a.entry, false, NULL, 0));
    a.entry->source = spelling;
    /* Existing self-contained wrappers transfer the context and discard only
     * the synthetic main() spelling, not the retained checked function body. */
    a.entry->destroy_context = nl_semantic_destroy;
    a.context = NULL;
    a.entry->source = NULL;
    nl_source_destroy(a.entry_source);
    a.entry_source = NULL;
    CHECK(nl_h_custody_validate(a.entry) &&
          emit(a.entry, true, baseline, bytes));
    a.entry->source = b.entry->source;
    CHECK(!nl_h_custody_validate(a.entry) && emit(a.entry, false, NULL, 0));
    a.entry->source = NULL;
    NLCheckedFragment *body = a.entry->bodies[0];
    const NLSource *source = body->source;
    body->source = NULL;
    CHECK(!nl_h_custody_validate(a.entry) && emit(a.entry, false, NULL, 0));
    body->source = source;
    CHECK(nl_h_custody_validate(a.entry) &&
          emit(a.entry, true, baseline, bytes));
    node_checked_destroy(&a);
    CHECK(emit(b.entry, true, baseline, bytes));
    node_checked_destroy(&b);
    nl_semantic_destroy(base);
    free(baseline);
    puts("C89 owned root entry spelling released; foreign root/body source "
         "refused; canonical A cleanup and genuine B healthy");
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && probe(argv[1]) ? 0 : 1;
}
