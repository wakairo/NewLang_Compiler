/* Only this test TU sees the private issued proof, not a new Production API. */
#define _POSIX_C_SOURCE 200809L
#include "semantic_check.c"
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>

bool p1184_c246a_emit(FILE *, const NLCheckedFragment *);

typedef struct {
    NLSource *source, *entry;
    NLParser *parser, *entry_parser;
    NLSyntaxTree *syntax, *entry_syntax;
    NLSemanticContext *world;
    NLCheckedFragment *root;
    const NLCheckedFragment *owner;
    const NLLinearBreakProof *proof;
} Fixture;

static void create(Fixture *a, const char *path)
{
    assert(nl_source_load(path, &a->source) == NL_SOURCE_OK);
    assert(nl_parser_create(a->source, &a->parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(a->parser, &a->syntax, NULL) ==
           NL_PARSE_OK);
    assert(nl_semantic_create(&a->world) == NL_CHECK_OK);
    const NLSyntaxTree *units[] = {a->syntax};
    assert(nl_semantic_register_function_unit(a->world, units, 1, NULL) ==
           NL_CHECK_OK);
    assert(nl_source_create("main()", 6, "C246a actual CHECK", &a->entry) ==
           NL_SOURCE_OK);
    assert(nl_parser_create(a->entry, &a->entry_parser) == NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(
               a->entry_parser, &a->entry_syntax, NULL) == NL_PARSE_OK);
    assert(nl_semantic_check_expression(a->world, a->entry_syntax, &a->root,
                                        NULL) == NL_CHECK_OK);
    a->owner = nl_checked_call_body(a->root, nl_checked_root(a->root));
    assert(a->owner && a->owner->linear_breaks);
    a->proof = a->owner->linear_breaks;
    assert(!a->proof->next && a->proof->current_cycle &&
           a->proof->current_cycle->helper && a->proof->sealed);
}

static void destroy(Fixture *a)
{
    nl_checked_destroy(a->root);
    nl_semantic_destroy(a->world);
    nl_syntax_tree_destroy(a->entry_syntax);
    nl_parser_destroy(a->entry_parser);
    nl_source_destroy(a->entry);
    nl_syntax_tree_destroy(a->syntax);
    nl_parser_destroy(a->parser);
    nl_source_destroy(a->source);
}

static void healthy(const Fixture *a)
{
    NLLinearBreakView view = {0};
    assert(nl_linear_break_validate(a->owner, a->proof->loop, &view) ==
           NL_CHECK_OK);
    assert(view.changing_current && view.incoming_call);
    assert(nl_linear_break_owner_validate(a->owner) == NL_CHECK_OK);
}

static void emits(const Fixture *a)
{
    FILE *stream = tmpfile();
    assert(stream && p1184_c246a_emit(stream, a->root));
    assert(fflush(stream) == 0 && ftell(stream) > 0);
    assert(fclose(stream) == 0);
}

static void rejected(const Fixture *a)
{
    NLLinearBreakView out;
    unsigned char before[sizeof out];
    memset(&out, 0xa5, sizeof out);
    memcpy(before, &out, sizeof out);
    assert(nl_linear_break_validate(a->owner, a->proof->loop, &out) ==
           NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    assert(!memcmp(before, &out, sizeof out));
    assert(nl_linear_break_owner_validate(a->owner) ==
           NL_CHECK_ANALYSIS_PRECISION_LIMIT);

    /* The real admission/emit pair must reject without touching its sink.
     * This checks consumer no-clobber, not a substitute for named CLI tests. */
    const char sentinel[] = "C246a KEEP original output\n";
    char content[sizeof sentinel];
    FILE *stream = tmpfile();
    assert(stream);
    assert(fwrite(sentinel, 1, sizeof sentinel, stream) == sizeof sentinel);
    assert(fflush(stream) == 0);
    struct stat saved, after;
    assert(fstat(fileno(stream), &saved) == 0);
    assert(!p1184_c246a_emit(stream, a->root));
    assert(fflush(stream) == 0 && ftell(stream) == (long)sizeof sentinel);
    assert(fstat(fileno(stream), &after) == 0);
    assert(saved.st_ino == after.st_ino && saved.st_size == after.st_size &&
           saved.st_mtim.tv_sec == after.st_mtim.tv_sec &&
           saved.st_mtim.tv_nsec == after.st_mtim.tv_nsec);
    rewind(stream);
    assert(fread(content, 1, sizeof content, stream) == sizeof content);
    assert(!memcmp(content, sentinel, sizeof sentinel));
    assert(fclose(stream) == 0);
}

int main(int argc, char **argv)
{
    assert(argc == 2 && sizeof(NLTypeId) > 1);
    Fixture a = {0}, b = {0};
    create(&a, argv[1]);
    create(&b, argv[1]);
    healthy(&a);
    healthy(&b);
    emits(&a);
    NLCurrentLoopProof *s = a.proof->current_cycle;
    NLSemanticContext *worlds[12];
    for (size_t i = 0; i < 6; ++i)
        worlds[i] = (NLSemanticContext *)s->graphs[i].world.world;
    worlds[6] = (NLSemanticContext *)a.owner->context;
    worlds[7] = a.proof->post;
    for (size_t i = 0; i < 4; ++i)
        worlds[8 + i] = (NLSemanticContext *)s->exits[i].world;
    size_t trials = 0;
    for (size_t slot = 0; slot < 12; ++slot) {
        NLSemanticContext *world = worlds[slot];
        assert(world && world->functions && s->helper_function &&
               s->helper_function <= world->function_count);
        NLFunctionEntry *table = world->functions;
        const size_t count = world->function_count;
        NLFunctionEntry *fn = &table[s->helper_function - 1];
        const NLFunctionEntry original = *fn;
        assert(original.count == 1 && original.parameters);
        for (size_t mode = 0; mode < 10; ++mode) {
            const size_t bytes = mode == 1 || mode == 2 ? sizeof(NLTypeId) : 1;
            unsigned char *foreign = malloc(bytes);
            assert(foreign);
            memset(foreign, 0xa5, bytes);
            if (mode == 2)
                memcpy(foreign, original.parameters, sizeof(NLTypeId));
            if (mode <= 5) {
                /* Never read through the challenged typed pointer here. A
                 * one-past pointer is formed within its live allocation. */
                fn->parameters =
                    (NLTypeId *)(mode == 1 ? foreign + bytes : foreign);
                if (mode == 3)
                    fn->parameters = NULL;
                if (mode == 4)
                    fn->count = 0;
                if (mode == 5)
                    fn->count = SIZE_MAX;
            } else if (mode == 6) {
                world->functions = NULL;
            } else if (mode == 7) {
                world->functions = (NLFunctionEntry *)foreign;
            } else if (mode == 8) {
                world->function_count = 0;
            } else {
                world->function_count = SIZE_MAX;
            }
            rejected(&a);
            healthy(&b);
            world->functions = table;
            world->function_count = count;
            *fn = original;
            free(foreign);
            healthy(&a);
            ++trials;
            printf("C246a slot=%zu mode=%zu reject/out/sink/retry=PASS\n", slot,
                   mode);
        }
    }
    assert(trials == 120);
    emits(&a);
    destroy(&a);
    healthy(&b);
    emits(&b);
    destroy(&b);
    a = (Fixture){0};
    create(&a, argv[1]);
    healthy(&a);
    emits(&a);
    destroy(&a);
    puts("C246a trials=120 B-survive/cleanup/fresh-CHECK-retry=PASS");
    return 0;
}
