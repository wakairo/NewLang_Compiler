/* Reuse P's source loader and host wrappers; no new proof or fault engine. */
#define main retained_p1463_main
#include "../p1463/probe.c"
#undef main

static NLCheckedNodeView *prefix_read, *terminal;
static void locate(NLCheckedFragment *f)
{
    for (size_t i = 1; i <= f->count; ++i) {
        NLCheckedNodeView *n = &f->nodes[i - 1];
        if (!prefix_read && n->kind == NL_CHECKED_FIELD_READ)
            prefix_read = n;
        if (n->owner_call.entry_proved)
            terminal = n;
        NLCheckedFragment *body =
            (NLCheckedFragment *)nl_checked_call_body(f, i);
        if (body)
            locate(body);
        if (n->kind == NL_CHECKED_MATCH)
            for (size_t a = 0; a < n->item_count; ++a)
                locate((NLCheckedFragment *)nl_checked_match_arm(f, i, a));
    }
}
static bool agreement(const char *path, const char *raw)
{
    Program p = {0};
    CHECK(ready(path, &p) && actual(&p));
    char *text = NULL;
    size_t length = 0;
    CHECK(nl_checked_c_node(p.artifact, &text, &length) == NL_NODE_C_OK);
    NLSource *expected = NULL;
    CHECK(nl_source_load(raw, &expected) == NL_SOURCE_OK);
    NLSourceView v;
    CHECK(nl_source_view(expected,
                         (NLSourceSpan){0, nl_source_length(expected)}, &v));
    CHECK(v.length == length && !memcmp(v.bytes, text, length));
    printf("P1529 genuine public API / CLI bytes agree: %zu\n", length);
    nl_source_destroy(expected);
    free(text);
    destroy(&p);
    return true;
}
static bool attacks(const char *path)
{
    Program p = {0}, foreign = {0};
    CHECK(ready(path, &p) && actual(&p) && ready(path, &foreign) &&
          actual(&foreign));
    locate(p.artifact);
    CHECK(prefix_read && terminal);
    NLCheckedNodeView saved = *prefix_read;
    prefix_read->field.child_fact = 0;
    bool bad = refused(&p, "prefix current fact");
    *prefix_read = saved;
    CHECK(bad && emit(&p, NL_NODE_C_OK));
    prefix_read->results[0].value = 0;
    bad = refused(&p, "prefix Copy result");
    *prefix_read = saved;
    CHECK(bad && emit(&p, NL_NODE_C_OK));
    struct NLHOriginOwner *origin = p.artifact->h_origin;
    p.artifact->h_origin = foreign.artifact->h_origin;
    bad = refused(&p, "foreign authentic origin world");
    p.artifact->h_origin = origin;
    CHECK(bad && emit(&p, NL_NODE_C_OK));
    saved = *terminal;
    terminal->owner_call.domain = 0;
    bad = refused(&p, "terminal original Domain");
    *terminal = saved;
    CHECK(bad && emit(&p, NL_NODE_C_OK));
    printf("P1529 four isolated artifact negatives / pristine retries PASS\n");
    destroy(&foreign);
    destroy(&p);
    return true;
}
int main(int argc, char **argv)
{
    if (argc == 4 && !strcmp(argv[1], "agree"))
        return agreement(argv[2], argv[3]) ? 0 : 1;
    if (argc == 3 && !strcmp(argv[1], "attack"))
        return attacks(argv[2]) ? 0 : 1;
    return 2;
}
