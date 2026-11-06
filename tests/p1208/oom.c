/* P-owned focused successor; retained P1184 mechanics, selected new call paths
 * only. */
#define main p1184_cli_main
#include "main.c"
#undef main
#include <assert.h>
#include <dirent.h>

typedef struct {
    NLSource *source, *entry;
    NLParser *parser, *entry_parser;
    NLSyntaxTree *syntax, *entry_syntax;
    NLSemanticContext *world;
    NLCheckedFragment *root;
    NLCheckedFragment *retained_owner;
    NLCheckedNodeId retained_loop;
} Input;
static void parse(Input *a, const char *path)
{
    assert(nl_source_load(path, &a->source) == NL_SOURCE_OK);
    assert(nl_parser_create(a->source, &a->parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(a->parser, &a->syntax, NULL) ==
           NL_PARSE_OK);
    assert(nl_source_create("main()", 6, "C263 actual source invocation",
                            &a->entry) == NL_SOURCE_OK);
    assert(nl_parser_create(a->entry, &a->entry_parser) == NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(
               a->entry_parser, &a->entry_syntax, NULL) == NL_PARSE_OK);
    assert(nl_semantic_create(&a->world) == NL_CHECK_OK);
}
static NLCheckStatus registration(Input *a)
{
    const NLSyntaxTree *units[] = {a->syntax};
    return nl_semantic_register_function_unit(a->world, units, 1, NULL);
}
static NLCheckStatus invocation(Input *a)
{
    return nl_semantic_check_expression(a->world, a->entry_syntax, &a->root,
                                        NULL);
}
static void dispose(Input *a)
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
static NLCheckedFragment *owner(Input *a)
{
    return (NLCheckedFragment *)nl_checked_call_body(a->root,
                                                     nl_checked_root(a->root));
}
static NLCheckedNodeId loop_id(Input *a)
{
    NLCheckedFragment *f = owner(a);
    assert(f);
    NLCheckedNodeId loop = 0;
    for (size_t i = 0; i < f->count; ++i)
        if (f->nodes[i].kind == NL_CHECKED_LOOP) {
            assert(!loop);
            loop = i + 1;
        }
    assert(loop);
    return loop;
}
static bool valid(Input *a, NLLinearBreakView *out)
{
    if (!a->retained_owner) {
        a->retained_owner = owner(a);
        if (!a->retained_owner)
            return false;
        a->retained_loop = loop_id(a);
    }
    return nl_linear_break_validate(a->retained_owner, a->retained_loop, out) ==
               NL_CHECK_OK &&
           out->changing_current && out->scalar_loop && out->incoming_call;
}

static bool enabled;
static size_t calls, fail_at;
static void *sites[8192];
static char kinds[8192];
static bool inject(char kind, void *pc)
{
    if (!enabled)
        return false;
    if (calls < sizeof sites / sizeof sites[0]) {
        sites[calls] = pc;
        kinds[calls] = kind;
    }
    return ++calls == fail_at;
}
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
void *__wrap_malloc(size_t n)
{
    return inject('m', __builtin_return_address(0)) ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t m)
{
    return inject('c', __builtin_return_address(0)) ? NULL
                                                    : __real_calloc(n, m);
}
void *__wrap_realloc(void *p, size_t n)
{
    return inject('r', __builtin_return_address(0)) ? NULL
                                                    : __real_realloc(p, n);
}
static void site_list(const char *stage, size_t total)
{
    assert(total <= sizeof sites / sizeof sites[0]);
    for (size_t i = 0; i < total; ++i)
        printf("C263 SITE stage=%s ordinal=%zu kind=%c pc=%p\n", stage, i + 1,
               kinds[i], sites[i]);
}
static size_t census(const char *path, bool actual)
{
    Input a = {0};
    parse(&a, path);
    if (actual)
        assert(registration(&a) == NL_CHECK_OK);
    calls = 0;
    fail_at = SIZE_MAX;
    enabled = true;
    NLCheckStatus status = actual ? invocation(&a) : registration(&a);
    enabled = false;
    assert(status == NL_CHECK_OK);
    const size_t total = calls;
    site_list(actual ? "actual" : "registration", total);
    dispose(&a);
    return total;
}
static void sentinel(const char *path)
{
    FILE *f = fopen(path, "wb");
    assert(f && fputs("C263 original named output\n", f) >= 0 &&
           fclose(f) == 0);
}

static void trial(const char *path, bool actual, size_t point)
{
    Input a = {0};
    parse(&a, path);
    if (actual)
        assert(registration(&a) == NL_CHECK_OK);
    NLCopyCallAnchor before = {0};
    assert(nl_copy_state_anchor(a.world, &before) == NL_CHECK_OK);
    const NLSemanticContext storage = *a.world;
    calls = 0;
    fail_at = point;
    enabled = true;
    NLCheckStatus status = actual ? invocation(&a) : registration(&a);
    enabled = false;
    assert(status == NL_CHECK_OUT_OF_MEMORY && calls >= point && !a.root);
    assert(nl_copy_state_matches(a.world, &before));
    assert(a.world->types == storage.types &&
           a.world->type_count == storage.type_count);
    assert(a.world->functions == storage.functions &&
           a.world->function_count == storage.function_count);
    assert(a.world->last_incarnation == storage.last_incarnation &&
           a.world->last_value_fact == storage.last_value_fact);
    nl_copy_state_destroy(&before);
    if (!actual)
        assert(registration(&a) == NL_CHECK_OK);
    assert(invocation(&a) == NL_CHECK_OK);
    NLLinearBreakView view;
    assert(valid(&a, &view));
    dispose(&a);
    printf("C263 reached stage=%s ordinal=%zu "
           "exact-OOM/rollback/no-publication/retry=PASS\n",
           actual ? "actual" : "registration", point);
}

static void named_selected(const char *path, const size_t *points, size_t count)
{
    char directory[] = "/tmp/p1208-named-XXXXXX", target[128];
    assert(mkdtemp(directory));
    assert(snprintf(target, sizeof target, "%s/out.c", directory) > 0);
    char *args[] = {"newlangc", "--emit-c17-to", target, (char *)path};
    sentinel(target);
    calls = 0;
    fail_at = SIZE_MAX;
    enabled = true;
    int status = p1184_cli_main(4, args);
    enabled = false;
    assert(status == 0);
    const size_t total = calls;
    if (!count)
        site_list("named", total);
    for (size_t i = 0; i < count; ++i) {
        sentinel(target);
        struct stat before, after;
        assert(stat(target, &before) == 0);
        calls = 0;
        fail_at = points[i];
        enabled = true;
        status = p1184_cli_main(4, args);
        enabled = false;
        assert(status != 0 && calls >= points[i]);
        assert(stat(target, &after) == 0 && before.st_ino == after.st_ino &&
               before.st_mtim.tv_sec == after.st_mtim.tv_sec &&
               before.st_mtim.tv_nsec == after.st_mtim.tv_nsec);
        FILE *f = fopen(target, "rb");
        char buffer[64] = {0};
        assert(f && fread(buffer, 1, sizeof buffer, f) ==
                        strlen("C263 original named output\n"));
        assert(fclose(f) == 0 &&
               !strcmp(buffer, "C263 original named output\n"));
        DIR *d = opendir(directory);
        assert(d);
        for (struct dirent *e = readdir(d); e; e = readdir(d))
            assert(!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..") ||
                   !strcmp(e->d_name, "out.c"));
        assert(closedir(d) == 0);
        assert(p1184_cli_main(4, args) == 0);
        printf("C263 reached stage=named ordinal=%zu "
               "failure/bytes-inode-mtime-no-clobber/temporary/retry=PASS\n",
               points[i]);
    }
    assert(unlink(target) == 0 && rmdir(directory) == 0);
    printf("C263 named total=%zu tested=%zu\n", total, count);
}

int main(int argc, char **argv)
{
    assert(argc == 3 || argc == 4);
    setvbuf(stdout, NULL, _IONBF, 0);
    if (!strcmp(argv[2], "census")) {
        assert(argc == 3);
        size_t reg = census(argv[1], false), actual = census(argv[1], true);
        printf("C263 totals registration=%zu actual=%zu\n", reg, actual);
        named_selected(argv[1], NULL, 0);
        return 0;
    }
    assert(argc == 4);
    size_t points[1024], count = 0, point = 0;
    FILE *f = fopen(argv[3], "r");
    assert(f);
    while (fscanf(f, "%zu", &point) == 1) {
        assert(point && count < sizeof points / sizeof points[0]);
        assert(!count || point > points[count - 1]);
        points[count++] = point;
    }
    assert(feof(f) && fclose(f) == 0 && count);
    if (!strcmp(argv[2], "named"))
        named_selected(argv[1], points, count);
    else {
        assert(!strcmp(argv[2], "actual") || !strcmp(argv[2], "registration"));
        for (size_t i = 0; i < count; ++i)
            trial(argv[1], !strcmp(argv[2], "actual"), points[i]);
    }
    printf("C263 selected OOM stage=%s trials=%zu PASS\n", argv[2], count);
    return 0;
}
