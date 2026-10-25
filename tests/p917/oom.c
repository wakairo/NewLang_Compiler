/* P-owned ACTUAL new snapshot allocation failures, not verifier scratch OOM. */
#define main p917_reference_main
#include "probe.c"
#undef main
enum {
    OFF,
    RECORD,
    PRE,
    POST
};
static int fault, stage;
static bool in_clone;
static size_t hits, requests;
void *__real_calloc(size_t, size_t);
void *__real_malloc(size_t);
NLCheckStatus __real_nl_sem_clone(const NLSemanticContext *,
                                  NLSemanticContext **);
NLDefinitionOwnedPending *
__real_nl_definition_owned_call(const NLCheckedFragment *, NLCheckedNodeId,
                                const NLCheckedNodeId[2], size_t,
                                const NLSemanticContext *);
void __real_nl_definition_owned_attach(NLDefinitionOwnedPending *,
                                       const NLCheckedFragment *,
                                       const NLSymbolId[2],
                                       const NLSemanticContext *);
void *__wrap_calloc(size_t n, size_t size)
{
    if (stage == PRE && fault == RECORD && !hits) {
        ++requests;
        ++hits;
        return NULL;
    }
    return __real_calloc(n, size);
}
void *__wrap_malloc(size_t size)
{
    if (in_clone && stage == fault && (fault == PRE || fault == POST) &&
        !hits) {
        REQUIRE(size == sizeof(NLSemanticContext));
        ++requests;
        ++hits;
        return NULL;
    }
    return __real_malloc(size);
}
NLCheckStatus __wrap_nl_sem_clone(const NLSemanticContext *c,
                                  NLSemanticContext **out)
{
    bool prior = in_clone;
    in_clone = stage == PRE || stage == POST;
    NLCheckStatus s = __real_nl_sem_clone(c, out);
    in_clone = prior;
    return s;
}
NLDefinitionOwnedPending *__wrap_nl_definition_owned_call(
    const NLCheckedFragment *f, NLCheckedNodeId call,
    const NLCheckedNodeId a[2], size_t function, const NLSemanticContext *c)
{
    stage = PRE;
    NLDefinitionOwnedPending *p =
        __real_nl_definition_owned_call(f, call, a, function, c);
    stage = OFF;
    return p;
}
void __wrap_nl_definition_owned_attach(NLDefinitionOwnedPending *p,
                                       const NLCheckedFragment *f,
                                       const NLSymbolId formals[2],
                                       const NLSemanticContext *c)
{
    stage = POST;
    __real_nl_definition_owned_attach(p, f, formals, c);
    stage = OFF;
}
int main(int argc, char **argv)
{
    REQUIRE(argc == 2);
    Input in = load(argv[1]);
    for (int mode = RECORD; mode <= POST; ++mode) {
        fault = mode;
        hits = requests = 0;
        NLDefinitionOwned *w = construct(&in, false);
        REQUIRE(hits == 1 && requests == 1 &&
                nl_definition_owned_status(w) == NL_CHECK_OUT_OF_MEMORY &&
                nl_definition_owned_count(w) == 0 &&
                !nl_definition_owned_link(w, 0));
        nl_definition_owned_destroy(w);
        fault = OFF;
        w = construct(&in,
                      false); /* SAME original context, source and parser. */
        REQUIRE(nl_definition_owned_status(w) == NL_CHECK_OK &&
                nl_definition_owned_count(w) == 2);
        for (size_t i = 0; i < 2; ++i) {
            NLDefinitionCallResult r;
            REQUIRE(nl_definition_owned_validate(w, i,
                                                 nl_definition_owned_link(w, i),
                                                 &r) == NL_CHECK_OK &&
                    r.pre_verified && r.return_old_verified && r.loan_closed);
        }
        nl_definition_owned_destroy(w);
        printf("new owned fault%d hit%zu requests%zu, zero partial records, "
               "same-context retry/replay OK; CHECK3 rollback kept\n",
               mode, hits, requests);
    }
    release(&in);
    return 0;
}
