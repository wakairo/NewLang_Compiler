/* R736 independently selected borrowed-view copy integrity attacks. */
#include "../support/node_checked.h"
#include "newlang/checked.h"
#include <stdio.h>
#include <string.h>
static unsigned producers, mutations;
static bool mutate_and_check(const NLCheckedFragment *f, size_t id,
                             const NLProducerLocalReturnView *orig) {
    for(unsigned k=0;k<26;++k) {
        NLProducerLocalReturnView v=*orig;
        switch(k) {
        case 0:v.declaration=0;break;
        case 1:v.construction=0;break;
        case 2:v.consumer=0;break;
        case 3:v.return_node=0;break;
        case 4:v.local=0;break;
        case 5:v.result.value=0;break;
        case 6:v.result.type=0;break;
        case 7:v.declaration++;break;
        case 8:v.construction++;break;
        case 9:v.consumer++;break;
        case 10:v.return_node++;break;
        case 11:v.local++;break;
        case 12:v.result.value++;break;
        case 13:v.result.type++;break;
        case 14:v.body=f;break;
        case 15:v.entry=nl_checked_context(f);break;
        case 16:v.returned=nl_checked_context(f);break;
        case 17:v.entry=orig->returned;break;
        case 18:v.returned=orig->entry;break;
        case 19:v.entry=orig->returned;v.returned=orig->entry;break;
        case 20:v.return_node=v.construction;break;
        case 21:v.consumer=v.return_node;break;
        case 22:v.construction=v.return_node;break;
        case 23:v.local=orig->local+127;break;
        case 24:v.result.value=orig->result.value+127;break;
        case 25:v.declaration=orig->return_node;break;
        }
        if(nl_checked_producer_local_return_valid(f,id,&v)) {
            fprintf(stderr,"R736_PUBLIC_TAMPER_ACCEPTED mutation=%u\n",k);
            return false;
        }
        if(!nl_checked_producer_local_return_valid(f,id,orig)) return false;
        ++mutations;
    }
    return true;
}
static bool walk(const NLCheckedFragment *f) {
    for(size_t id=1;id<=nl_checked_node_count(f);++id) {
        const NLCheckedNodeView *n=nl_checked_node_view(f,id);
        if(n->producer.entry_proved) {
            NLProducerLocalReturnView v;
            if(!nl_checked_producer_local_return_view(f,id,&v) ||
               !nl_checked_producer_local_return_valid(f,id,&v) ||
               !nl_checked_producer_valid(f,id) ||
               !mutate_and_check(f,id,&v)) return false;
            NLSemanticBindingView binding;
            NLSemanticPlaceView before,after;
            if(!nl_semantic_binding_view(v.returned,v.local,&binding) ||
               binding.availability!=NL_CONSUMED ||
               !nl_semantic_place_view(v.entry,n->producer.root,&before) ||
               !nl_semantic_place_view(v.returned,n->producer.root,&after) ||
               !before.live || !after.live ||
               before.incarnation!=after.incarnation ||
               before.placement.region!=after.placement.region ||
               before.governing_domain!=after.governing_domain) return false;
            ++producers;
            printf("R736_SOURCE_ISSUED producer=%zu local=%zu return=%zu original_root=%zu original_region=%zu domain=%zu tamper_rejected=%u\n",
                   id,v.local,v.return_node,n->producer.root,
                   n->producer.range.region,n->producer.domain,26u);
        }
        const NLCheckedFragment *body=nl_checked_call_body(f,id);
        if(body && !walk(body)) return false;
        if(n->kind==NL_CHECKED_MATCH)
            for(size_t a=0;a<n->item_count;++a)
                if(!walk(nl_checked_match_arm(f,id,a)))return false;
    }
    return true;
}
int main(int argc,char **argv) {
    TestNode n={0};
    if(argc!=2 || !node_checked_load(argv[1],&n) || !walk(n.entry) ||
       producers!=1 || mutations!=26) return 1;
    node_checked_destroy(&n);
    puts("R736_PUBLIC_PASS");
    return 0;
}
