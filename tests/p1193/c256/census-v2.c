/* Passive P-owned matched census. Compile the exact checker in this TU only
 * to inspect its private retained representation; no compiler API is added. */
#include "semantic_check.c"
#include <assert.h>

static struct { void *p; size_t bytes; } allocations[16384];
static size_t calls, live_bytes, peak_bytes, live_blocks, peak_blocks;
static size_t stage;
static size_t c256_at(void *p) {
    for(size_t i=0;i<16384;++i) if(allocations[i].p==p) return i;
    return 16384;
}
static void c256_add(void *p,size_t bytes,char kind,void *pc) {
    if(!p) return;
    size_t i=c256_at(NULL); assert(i<16384);
    allocations[i].p=p;allocations[i].bytes=bytes;
    live_bytes+=bytes;++live_blocks;
    if(live_bytes>peak_bytes)peak_bytes=live_bytes;
    if(live_blocks>peak_blocks)peak_blocks=live_blocks;
    printf("SITE stage=%zu ordinal=%zu kind=%c bytes=%zu pc=%p\n",stage,++calls,kind,bytes,pc);
}
static void c256_remove_at(size_t i) {
    if(i==16384)return;
    live_bytes-=allocations[i].bytes;--live_blocks;
    allocations[i].p=NULL;allocations[i].bytes=0;
}
void *__real_malloc(size_t);void *__real_calloc(size_t,size_t);
void *__real_realloc(void *,size_t);void __real_free(void *);
void *__wrap_malloc(size_t n) {void *p=__real_malloc(n);c256_add(p,n,'m',__builtin_return_address(0));return p;}
void *__wrap_calloc(size_t n,size_t z) {assert(!z||n<=SIZE_MAX/z);void *p=__real_calloc(n,z);c256_add(p,n*z,'c',__builtin_return_address(0));return p;}
void *__wrap_realloc(void *p,size_t n) {size_t i=c256_at(p);void *q=__real_realloc(p,n);if(q||!n)c256_remove_at(i);if(q)c256_add(q,n,'r',__builtin_return_address(0));return q;}
void __wrap_free(void *p) {if(p)c256_remove_at(c256_at(p));__real_free(p);}
static NLSemanticContext *contexts[1024];
static size_t live_contexts,peak_contexts,unknown_destroy;
static void c256_remember(NLSemanticContext *p) {
    for(size_t i=0;i<1024;++i)if(contexts[i]==p)return;
    for(size_t i=0;i<1024;++i)if(!contexts[i]){contexts[i]=p;++live_contexts;if(live_contexts>peak_contexts)peak_contexts=live_contexts;return;}
    assert(0);
}
static bool c256_forget(NLSemanticContext *p) {
    for(size_t i=0;i<1024;++i)if(contexts[i]==p){contexts[i]=NULL;--live_contexts;return true;}
    return false;
}
NLCheckStatus __real_nl_semantic_create(NLSemanticContext **);
NLCheckStatus __wrap_nl_semantic_create(NLSemanticContext **p){NLCheckStatus s=__real_nl_semantic_create(p);if(s==NL_CHECK_OK)c256_remember(*p);return s;}
NLCheckStatus __real_nl_sem_clone(const NLSemanticContext *,NLSemanticContext **);
NLCheckStatus __wrap_nl_sem_clone(const NLSemanticContext *c,NLSemanticContext **p){NLCheckStatus s=__real_nl_sem_clone(c,p);if(s==NL_CHECK_OK)c256_remember(*p);return s;}
void __real_nl_semantic_destroy(NLSemanticContext *);
void __wrap_nl_semantic_destroy(NLSemanticContext *p){if(p&&!c256_forget(p))++unknown_destroy;__real_nl_semantic_destroy(p);}
void __real_nl_sem_commit(NLSemanticContext *,NLSemanticContext *);
void __wrap_nl_sem_commit(NLSemanticContext *c,NLSemanticContext *p){assert(c256_forget(p));__real_nl_sem_commit(c,p);}
static const NLCheckedFragment *visited[128];
static const NLSemanticContext *return_worlds[128];
static size_t graphs,nodes,return_edges,return_states,result_seals;
static void c256_walk(const NLCheckedFragment *f) {
    assert(f && graphs<128);
    for(size_t i=0;i<graphs;++i)if(visited[i]==f)return;
    visited[graphs++]=f;nodes+=f->count;
    for(size_t i=0;i<f->count;++i)
        if(f->nodes[i].kind==NL_CHECKED_REGISTERED_CALL && nl_h_custody_result_valid(f,i+1))++result_seals;
    for(size_t i=0;i<nl_control_exits_count(f->exits);++i){
        const NLControlExitView *e=nl_control_exit_view(f->exits,i);
        if(e->kind!=NL_EXIT_RETURN)continue;
        ++return_edges;const NLSemanticContext *w=nl_control_state_context((NLControlState *)e->state);
        bool found=false;for(size_t j=0;j<return_states;++j)found|=return_worlds[j]==w;
        if(!found){assert(return_states<128);return_worlds[return_states++]=w;}
    }
    for(size_t i=0;i<f->arm_count;++i)c256_walk(f->arms[i].artifact);
    for(size_t i=0;i<f->body_count;++i)c256_walk(f->bodies[i]);
}
static void c256_report(const char *label) {
    printf("C256 MEMORY stage=%s alloc_ordinals=%zu live_bytes=%zu peak_bytes=%zu live_blocks=%zu peak_blocks=%zu live_contexts=%zu peak_contexts=%zu unknown_destroy=%zu\n",label,calls,live_bytes,peak_bytes,live_blocks,peak_blocks,live_contexts,peak_contexts,unknown_destroy);
}
int main(int argc,char **argv) {
    assert(argc==2);setvbuf(stdout,NULL,_IONBF,0);
    NLSource *source=NULL,*entry=NULL;NLParser *p=NULL,*q=NULL;
    NLSyntaxTree *unit=NULL,*call=NULL;NLSemanticContext *world=NULL;NLCheckedFragment *root=NULL;
    assert(nl_source_load(argv[1],&source)==NL_SOURCE_OK);
    assert(nl_parser_create(source,&p)==NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(p,&unit,NULL)==NL_PARSE_OK);
    assert(nl_semantic_create(&world)==NL_CHECK_OK);
    stage=1;size_t before=calls;const NLSyntaxTree *units[]={unit};
    assert(nl_semantic_register_function_unit(world,units,1,NULL)==NL_CHECK_OK);
    c256_report("registration");size_t registration_calls=calls-before;
    assert(nl_source_create("main()",6,"C256 matched census",&entry)==NL_SOURCE_OK);
    assert(nl_parser_create(entry,&q)==NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(q,&call,NULL)==NL_PARSE_OK);
    stage=2;before=calls;
    assert(nl_semantic_check_expression(world,call,&root,NULL)==NL_CHECK_OK);
    c256_report("issued");size_t call_calls=calls-before;
    assert(nl_h_custody_validate(root));
    const NLCheckedFragment *body=nl_checked_call_body(root,root->root);assert(body);
    stage=3;before=calls;
    size_t records=0,clones=0,anchors=0,inline_images=0,record_bytes=0,clone_bytes=0,anchor_bytes=0;
    for(NLLinearBreakProof *b=body->linear_breaks;b;b=b->next){
        NLLinearBreakView v;assert(nl_linear_break_validate(body,b->loop,&v)==NL_CHECK_OK);
#ifdef C256_CANDIDATE
        if(b->current_cycle){
            NLCurrentLoopProof *s=b->current_cycle;records+=s->return_count;
            record_bytes+=s->return_count*sizeof(*s->returns);
            for(size_t i=0;i<s->return_count;++i){
                CurrentReturnSeal *r=&s->returns[i];
                assert(r->world.copy);++clones;++inline_images;
                size_t z=c256_at(r->world.copy);assert(z<16384);clone_bytes+=allocations[z].bytes;
                const NLCopyCallAnchor *a=&r->world.anchor;
                const void *parts[]={a->places,a->values,a->scopes,a->bindings};
                for(size_t j=0;j<4;++j)if(parts[j]){z=c256_at((void *)parts[j]);assert(z<16384);++anchors;anchor_bytes+=allocations[z].bytes;}
            }
        }
#endif
    }
    size_t validate_calls=calls-before;c256_walk(root);
    c256_report("validated");
    printf("C256 RETAINED graphs=%zu nodes=%zu numeric_result_seals=%zu core_return_edges=%zu core_unique_return_contexts=%zu private_return_records=%zu snapshot_heap_contexts=%zu inline_context_images=%zu anchor_arrays=%zu record_bytes=%zu clone_top_bytes=%zu anchor_bytes=%zu context_size=%zu proof_size=%zu\n",graphs,nodes,result_seals,return_edges,return_states,records,clones,inline_images,anchors,record_bytes,clone_bytes,anchor_bytes,sizeof(NLSemanticContext),sizeof(NLCurrentLoopProof));
    printf("C256 ORDINALS registration=%zu actual=%zu validator=%zu\n",registration_calls,call_calls,validate_calls);
    stage=4;nl_checked_destroy(root);nl_semantic_destroy(world);
    nl_syntax_tree_destroy(call);nl_parser_destroy(q);nl_source_destroy(entry);
    nl_syntax_tree_destroy(unit);nl_parser_destroy(p);nl_source_destroy(source);
    c256_report("destroyed");assert(!live_contexts&&!unknown_destroy&&!live_blocks&&!live_bytes);
    return 0;
}
