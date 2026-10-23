/* R #780: independent fault-site reachability, rollback, and named CLI staging.
 * Only test-owned malloc/calloc/realloc wrappers inject one real NULL, never
 * manufacture compiler rights. Uses actual source -> normal checked entry.
 */
#define main r780_compiler_entry_unused
#include "../../src/main.c"
#undef main
#include "../support/node_checked.h"
#include "newlang/checked_c_node.h"
#include "newlang/copy_body.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static bool armed, did_fail;
static size_t seen, at;
void *__real_malloc(size_t n);
void *__real_calloc(size_t n,size_t s);
void *__real_realloc(void *p,size_t n);
static bool triggered(void) {
    if(!armed)return false;
    size_t now=seen++;
    if(now==at){did_fail=true;return true;}
    return false;
}
void *__wrap_malloc(size_t n){return triggered()?NULL:__real_malloc(n);}
void *__wrap_calloc(size_t n,size_t s){return triggered()?NULL:__real_calloc(n,s);}
void *__wrap_realloc(void *p,size_t n){return triggered()?NULL:__real_realloc(p,n);}
typedef struct {
    NLCopyBodyParameters copy;
    NLProducerLocalReturnView producer;
    bool have_copy,have_producer;
} Worlds;
static bool collect(const NLCheckedFragment *f,Worlds *out)
{
    for(size_t id=1;id<=nl_checked_node_count(f);id++){
        NLCopyBodyParameters p={0};
        if(!out->have_copy && nl_checked_copy_body_parameters(f,id,&p)){
            out->have_copy=true;out->copy=p;
        }
        const NLCheckedNodeView *node=nl_checked_node_view(f,id);
        if(!out->have_producer && node->producer.entry_proved){
            NLProducerLocalReturnView v={0};
            CHECK(nl_checked_producer_local_return_view(f,id,&v) &&
                  nl_checked_producer_local_return_valid(f,id,&v));
            out->have_producer=true;out->producer=v;
        }
        if(node->kind==NL_CHECKED_MATCH)
            for(size_t a=0;a<node->item_count;a++){
                const NLCheckedFragment *arm=nl_checked_match_arm(f,id,a);
                CHECK(arm && collect(arm,out));
            }
        const NLCheckedFragment *body=nl_checked_call_body(f,id);
        if(body)CHECK(collect(body,out));
    }
    return true;
}
static bool unchanged(char *text,const char *expected)
{
    FILE *f=fopen(text,"rb");
    CHECK(f!=NULL);
    char buf[80]={0};
    size_t n=fread(buf,1,sizeof(buf),f);
    CHECK(!ferror(f) && fclose(f)==0);
    CHECK(n==strlen(expected) && memcmp(buf,expected,n)==0);
    return true;
}
static bool save(char *path,const char *content)
{
    FILE *f=fopen(path,"wb");
    CHECK(f!=NULL);
    CHECK(fwrite(content,1,strlen(content),f)==strlen(content));
    CHECK(fclose(f)==0);
    return true;
}
static bool main_test(const char *source)
{
    TestNode n={0};
    CHECK(node_checked_load(source,&n));
    Worlds w={0};
    CHECK(collect(n.entry,&w) && w.have_copy && w.have_producer);
    NLSemanticContext *contexts[5]={n.context,
        (NLSemanticContext *)w.copy.entry,
        (NLSemanticContext *)w.copy.handoff,
        (NLSemanticContext *)w.producer.entry,
        (NLSemanticContext *)w.producer.returned};
    TestState *states=calloc(5,sizeof(*states));
    CHECK(states);
    for(size_t i=0;i<5;i++) CHECK(test_state(contexts[i],&states[i]));
    char *good=NULL;
    size_t good_bytes=0;
    CHECK(nl_checked_c_node(n.entry,&good,&good_bytes)==NL_NODE_C_OK &&
          good && good_bytes>500);
    size_t total=0;
    at=SIZE_MAX; seen=0;did_fail=false;armed=true;
    char *first=NULL;size_t first_size=0;
    NLNodeCStatus result=nl_checked_c_node(n.entry,&first,&first_size);
    armed=false;total=seen;
    CHECK(result==NL_NODE_C_OK && !did_fail && first &&
          first_size==good_bytes && memcmp(first,good,good_bytes)==0);
    free(first);
    CHECK(total>30);
    const size_t where[]={0,1,2,3,4,5,total/4,total/2,3*total/4,total-1};
    size_t count_oom=0,count_unsupported=0;
    for(size_t j=0;j<sizeof(where)/sizeof(where[0]);j++){
        at=where[j];seen=0;did_fail=false;armed=true;
        char *partial=NULL;size_t length=519;
        result=nl_checked_c_node(n.entry,&partial,&length);
        armed=false;
        CHECK(did_fail && partial==NULL && length==519 &&
              (result==NL_NODE_C_OUT_OF_MEMORY ||
               result==NL_NODE_C_UNSUPPORTED));
        if(result==NL_NODE_C_OUT_OF_MEMORY)count_oom++;
        else count_unsupported++;
        for(size_t i=0;i<5;i++)CHECK(test_unchanged(contexts[i],&states[i]));
        char *retry=NULL;size_t retry_bytes=0;
        CHECK(nl_checked_c_node(n.entry,&retry,&retry_bytes)==NL_NODE_C_OK &&
              retry_bytes==good_bytes && memcmp(retry,good,good_bytes)==0);
        free(retry);
        printf("R780_BACKEND_FAULT at=%zu reached=1 status=%d output_unchanged=1 five_worlds_unchanged=1 retry_same=1\n",
               at,(int)result);
    }
    CHECK(count_oom>0);
    printf("R780_BACKEND summary opportunities=%zu injected=%zu oom=%zu unsupported=%zu\n",
        total,sizeof(where)/sizeof(where[0]),count_oom,count_unsupported);
    /* Exercise the actual named-output staging path in source compiler,
       not a fake stream; re-verify old bytes after each reached fault. */
    char dir[]="/tmp/r780-named-XXXXXX";
    CHECK(mkdtemp(dir)!=NULL);
    char file[320]={0};
    CHECK(snprintf(file,sizeof(file),"%s/output.c",dir)>0);
    const char sentinel[]="R780-SAVED-C17-KEEP";
    CHECK(save(file,sentinel));
    at=SIZE_MAX;seen=0;did_fail=false;armed=true;
    int cli=emit_c17_to(file,source);
    armed=false;
    size_t cli_total=seen;
    CHECK(cli==0 && !did_fail && cli_total>30);
    const size_t sites[]={0,1,2,3,4,5,cli_total/8,cli_total/4,
       cli_total/2,3*cli_total/4,cli_total-2,cli_total-1};
    size_t fail_count=0,resources=0,unsupported=0;
    for(size_t j=0;j<sizeof(sites)/sizeof(sites[0]);j++){
        CHECK(save(file,sentinel));
        at=sites[j];seen=0;did_fail=false;armed=true;
        cli=emit_c17_to(file,source);
        armed=false;
        CHECK(did_fail && cli>0 && cli<5 && unchanged(file,sentinel));
        fail_count++;
        if(cli==1)resources++;
        if(cli==4)unsupported++;
        for(size_t i=0;i<5;i++)CHECK(test_unchanged(contexts[i],&states[i]));
        CHECK(emit_c17_to(file,source)==0);
        FILE *f=fopen(file,"rb");
        CHECK(f!=NULL);
        CHECK(fseek(f,0,SEEK_END)==0 && ftell(f)==(long)good_bytes);
        CHECK(fclose(f)==0);
        printf("R780_NAMED_FAULT at=%zu reached=1 CLI=%d sentinel_preserved=1 retry_success=1\n",
            at,cli);
    }
    CHECK(fail_count==12 && resources>0);
    printf("R780_NAMED summary opportunities=%zu injected=%zu resource=%zu unsupported=%zu\n",
          cli_total,fail_count,resources,unsupported);
    CHECK(unlink(file)==0 && rmdir(dir)==0);
    free(good);free(states);node_checked_destroy(&n);
    return true;
}
int main(int argc,char **argv)
{
    return argc==2 && main_test(argv[1])?0:1;
}
