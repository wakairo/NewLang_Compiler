/* Bounded Checked-C representation, not a NewLang ABI. */
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <limits.h>
_Static_assert(CHAR_BIT == 8, "NewLang C17 requires 8-bit bytes");
_Static_assert(UINT8_MAX == 255 && sizeof(uint8_t) == 1 && ((uint8_t)-1) == 255, "NewLang C17 requires exact 8-bit uint8_t");
typedef struct nl_node nl_node;
typedef struct { unsigned tag; const nl_node *ptr; } nl_node_option;
struct nl_node { nl_node_option f0; uint8_t f1; };
#ifdef NEWLANG_NODE_OBSERVER
#include NEWLANG_NODE_OBSERVER
#else
#define NL_NODE_BIND(...) ((void)0)
#define NL_NODE_REPLACE(...) ((void)0)
#define NL_NODE_ARM(...) ((void)0)
#define NL_NODE_RELOAN(...) ((void)0)
#define NL_NODE_FINISH() ((void)0)
#endif
_Static_assert(sizeof(nl_node)==24 && _Alignof(nl_node)==8, "checked H target layout");
_Static_assert(sizeof(void*)==8 && offsetof(nl_node,f0)==0 && offsetof(nl_node,f1)==16, "private member plan");
_Static_assert(sizeof(nl_node_option)==16 && _Alignof(nl_node_option)==8 && _Generic(((nl_node *)0)->f0,nl_node_option:1,default:0) && _Generic(((nl_node *)0)->f1,uint8_t:1,default:0) && _Generic(((nl_node_option *)0)->ptr,const nl_node *:1,default:0), "private member types");
typedef struct {void *handle;} nl_allocation;
typedef struct {unsigned char *bytes;size_t length;} nl_storage;
typedef struct {unsigned char *bytes;size_t length;} nl_slot;
typedef struct {size_t token;} nl_domain;
typedef struct {const nl_node *ptr;nl_allocation allocation;nl_domain domain;} nl_live_tail;
typedef struct {unsigned tag;nl_live_tail packet;} nl_custody;
typedef struct {nl_allocation allocation;nl_storage raw;} nl_backing;
#ifdef NEWLANG_HEAP_OBSERVER
#include NEWLANG_HEAP_OBSERVER
#else
#define NL_HEAP_TRIAL(...) ((void)0)
#define NL_HEAP_ARM(...) ((void)0)
#define NL_HEAP_DOMAIN(...) ((void)0)
#define NL_HEAP_SLOT(...) ((void)0)
#define NL_HEAP_INITIALIZE(...) ((void)0)
#define NL_HEAP_RELOAN(...) ((void)0)
#define NL_HEAP_ROOT(...) ((void)0)
#define NL_HEAP_FIELD(...) ((void)0)
#define NL_HEAP_COPY(...) ((void)0)
#define NL_HEAP_CHANGE(...) ((void)0)
#define NL_HEAP_SCOPE_END(...) ((void)0)
#define NL_HEAP_END(...) ((void)0)
#define NL_HEAP_RAW(...) ((void)0)
#define NL_HEAP_FINALIZE(...) ((void)0)
#define NL_HEAP_RELEASE(...) ((void)0)
#define NL_HEAP_FINISH() ((void)0)
#define NL_HEAP_HANDOFF(...) ((void)0)
#define NL_HEAP_RECEIVER_ENTER(...) ((void)0)
#define NL_HEAP_RECEIVER_EXIT() ((void)0)
#define NL_HEAP_RETURNED(...) ((void)0)
#define NL_LIVE_SEND(...) ((void)0)
#define NL_LIVE_ENTER(...) ((void)0)
#define NL_LIVE_RETURN(...) ((void)0)
#define NL_LIVE_RECEIVE(...) ((void)0)
#endif
#ifndef NL_CUSTODY_ENTER
#define NL_CUSTODY_ENTER(...) ((void)0)
#define NL_CUSTODY_STORED(...) ((void)0)
#define NL_CUSTODY_RETURNED(...) ((void)0)
#define NL_CUSTODY_EXTRACT(...) ((void)0)
#define NL_CUSTODY_NONE(...) ((void)0)
#define NL_CUSTODY_POLICY(...) ((void)0)
#define NL_CUSTODY_LOAN(...) ((void)0)
#endif
typedef struct {uint8_t f0,f1;} nl_current_record;
_Static_assert(SIZE_MAX==UINTMAX_C(18446744073709551615),"NewLang usize target");
#ifdef NEWLANG_CURRENT_LOOP_OBSERVER
#include NEWLANG_CURRENT_LOOP_OBSERVER
#endif
#ifndef NL_CURRENT_PHASE
#define NL_CURRENT_PHASE(...) ((void)0)
#endif
#ifndef NL_CURRENT_LOCAL
#define NL_CURRENT_LOCAL(...) ((void)0)
#endif
#ifndef NL_CURRENT_RESULT
#define NL_CURRENT_RESULT(...) ((void)0)
#endif
#ifndef NL_MIXED_COPY_BIND
#define NL_MIXED_COPY_BIND(...) ((void)0)
#endif
#ifndef NL_MIXED_COPY_STORE
#define NL_MIXED_COPY_STORE(...) ((void)0)
#endif
#ifndef NL_LOOP_READ
#define NL_LOOP_READ(...) ((void)0)
#endif
static void nl_owner_11(const nl_node *nl_b_12_34,nl_allocation nl_b_12_35,nl_domain nl_b_12_36) {
NL_HEAP_RECEIVER_ENTER(nl_b_12_34,nl_b_12_35.handle,nl_b_12_36.token,12,34,35,36,&nl_b_12_35,&nl_b_12_36);
{ const nl_domain *nl_b_13_37 = &nl_b_12_36;
(void)nl_b_13_37;
const nl_node * nl_v_75 = nl_b_12_34;
const nl_domain * nl_v_76 = nl_b_13_37;
const nl_node * nl_v_77 = nl_v_75;
NL_HEAP_ROOT(nl_v_77,nl_v_76->token,0,19,22,6);
(void)nl_v_76;
const nl_node * nl_b_13_38 = nl_v_77;
(void)nl_b_13_38;
NL_NODE_BIND(13, 38, 4, &nl_b_13_38);
NL_HEAP_SCOPE_END(6);
}
nl_slot nl_v_78;
{ const nl_domain *nl_b_14_39 = &nl_b_12_36;
(void)nl_b_14_39;
const nl_node * nl_v_79 = nl_b_12_34;
NL_HEAP_END(nl_v_79,nl_b_14_39->token);
nl_slot nl_v_80 = {(unsigned char *)nl_v_79,24};
nl_v_78 = nl_v_80;
}
nl_slot nl_b_12_40 = nl_v_78;
(void)nl_b_12_40;
NL_NODE_BIND(12, 40, 4, &nl_b_12_40);
nl_slot nl_v_81 = nl_b_12_40;
nl_b_12_40 = (nl_slot){0};
nl_storage nl_v_82 = {nl_v_81.bytes,nl_v_81.length};
NL_HEAP_RAW(nl_v_82.bytes,nl_v_82.length);
nl_storage nl_b_12_41 = nl_v_82;
(void)nl_b_12_41;
NL_NODE_BIND(12, 41, 4, &nl_b_12_41);
nl_domain nl_v_83 = nl_b_12_36;
nl_b_12_36 = (nl_domain){0};
NL_HEAP_FINALIZE(nl_v_83.token);
(void)nl_v_83;
nl_allocation nl_v_84 = nl_b_12_35;
nl_b_12_35 = (nl_allocation){0};
nl_storage nl_v_85 = nl_b_12_41;
nl_b_12_41 = (nl_storage){0};
NL_HEAP_RELEASE(nl_v_84.handle,nl_v_85.bytes,nl_v_85.length);
free(nl_v_84.handle);
nl_v_84.handle=NULL;
nl_v_85.bytes=NULL;
(void)nl_v_85;
NL_HEAP_RECEIVER_EXIT();
}
static nl_live_tail nl_producer_13(nl_node_option * nl_b_11_26,const nl_node * nl_b_11_27,nl_allocation nl_b_11_28,nl_domain nl_b_11_29) {
NL_LIVE_ENTER(nl_b_11_26,nl_b_11_27,nl_b_11_28.handle,nl_b_11_29.token);
nl_node_option * nl_v_64 = nl_b_11_26;
nl_node_option nl_v_65 = { 0, NULL };
nl_node_option nl_v_66 = *nl_v_64;
*nl_v_64 = nl_v_65;
NL_HEAP_CHANGE(nl_v_64,&nl_v_66,2);
(void)nl_v_66;
const nl_node * nl_v_67 = nl_b_11_27;
nl_allocation nl_v_68 = nl_b_11_28;
nl_b_11_28 = (nl_allocation){0};
nl_domain nl_v_69 = nl_b_11_29;
nl_b_11_29 = (nl_domain){0};
nl_live_tail nl_v_70 = {nl_v_67,nl_v_68,nl_v_69};
NL_LIVE_RETURN(nl_v_70);
return nl_v_70;
}
int main(void) {
nl_current_record nl_b_0_1={.f0=41,.f1=167,};
NL_MIXED_COPY_BIND(0,1,&nl_b_0_1.f0,&nl_b_0_1.f1);
uint8_t nl_v_1;
void *nl_heap_2 = malloc(24);
NL_HEAP_TRIAL(nl_heap_2,24,8);
if (nl_heap_2 == NULL) {
NL_HEAP_ARM(1);
uint8_t nl_v_3 = 27;
nl_v_1 = nl_v_3;
}
if (nl_heap_2 != NULL) {
NL_HEAP_ARM(2);
nl_backing nl_b_2_2 = { {nl_heap_2}, {(unsigned char *)nl_heap_2,24} };
nl_backing nl_v_4 = nl_b_2_2;
nl_b_2_2 = (nl_backing){0};
nl_allocation nl_b_2_3 = nl_v_4.allocation;
(void)nl_b_2_3;
nl_storage nl_b_2_4 = nl_v_4.raw;
(void)nl_b_2_4;
nl_v_4 = (nl_backing){0};
nl_allocation nl_v_5 = nl_b_2_3;
nl_b_2_3 = (nl_allocation){0};
nl_allocation nl_b_2_5 = nl_v_5;
(void)nl_b_2_5;
NL_NODE_BIND(2, 5, 4, &nl_b_2_5);
nl_storage nl_v_6 = nl_b_2_4;
nl_b_2_4 = (nl_storage){0};
nl_slot nl_v_7 = {nl_v_6.bytes,nl_v_6.length};
NL_HEAP_SLOT(nl_v_7.bytes,nl_v_7.length);
nl_slot nl_b_2_6 = nl_v_7;
(void)nl_b_2_6;
NL_NODE_BIND(2, 6, 4, &nl_b_2_6);
nl_domain nl_v_8 = {1};
NL_HEAP_DOMAIN(nl_v_8.token);
nl_domain nl_b_2_7 = nl_v_8;
(void)nl_b_2_7;
NL_NODE_BIND(2, 7, 4, &nl_b_2_7);
const nl_node * nl_v_9;
{ const nl_domain *nl_b_3_8 = &nl_b_2_7;
(void)nl_b_3_8;
nl_slot nl_v_10 = nl_b_2_6;
nl_b_2_6 = (nl_slot){0};
nl_node_option nl_v_11 = { 0, NULL };
uint8_t nl_v_12 = 19;
nl_node nl_v_13 = { nl_v_11, nl_v_12 };
const nl_domain * nl_v_14 = nl_b_3_8;
*(nl_node *)nl_v_10.bytes = nl_v_13;
const nl_node *nl_v_15 = (nl_node *)nl_v_10.bytes;
NL_HEAP_INITIALIZE(nl_v_15,nl_v_14->token);
(void)nl_v_14;
nl_v_9 = nl_v_15;
}
const nl_node * const nl_b_2_9 = nl_v_9;
(void)nl_b_2_9;
NL_NODE_BIND(2, 9, 2, &nl_b_2_9);
uint8_t nl_v_16;
void *nl_heap_17 = malloc(24);
NL_HEAP_TRIAL(nl_heap_17,24,8);
if (nl_heap_17 == NULL) {
NL_HEAP_ARM(1);
nl_slot nl_v_18;
{ const nl_domain *nl_b_5_10 = &nl_b_2_7;
(void)nl_b_5_10;
const nl_node * nl_v_19 = nl_b_2_9;
NL_HEAP_END(nl_v_19,nl_b_5_10->token);
nl_slot nl_v_20 = {(unsigned char *)nl_v_19,24};
nl_v_18 = nl_v_20;
}
nl_slot nl_b_4_11 = nl_v_18;
(void)nl_b_4_11;
NL_NODE_BIND(4, 11, 4, &nl_b_4_11);
nl_slot nl_v_21 = nl_b_4_11;
nl_b_4_11 = (nl_slot){0};
nl_storage nl_v_22 = {nl_v_21.bytes,nl_v_21.length};
NL_HEAP_RAW(nl_v_22.bytes,nl_v_22.length);
nl_storage nl_b_4_12 = nl_v_22;
(void)nl_b_4_12;
NL_NODE_BIND(4, 12, 4, &nl_b_4_12);
nl_domain nl_v_23 = nl_b_2_7;
nl_b_2_7 = (nl_domain){0};
NL_HEAP_FINALIZE(nl_v_23.token);
(void)nl_v_23;
nl_allocation nl_v_24 = nl_b_2_5;
nl_b_2_5 = (nl_allocation){0};
nl_storage nl_v_25 = nl_b_4_12;
nl_b_4_12 = (nl_storage){0};
NL_HEAP_RELEASE(nl_v_24.handle,nl_v_25.bytes,nl_v_25.length);
free(nl_v_24.handle);
nl_v_24.handle=NULL;
nl_v_25.bytes=NULL;
(void)nl_v_25;
uint8_t nl_v_26 = 83;
nl_v_16 = nl_v_26;
}
if (nl_heap_17 != NULL) {
NL_HEAP_ARM(2);
nl_backing nl_b_6_10 = { {nl_heap_17}, {(unsigned char *)nl_heap_17,24} };
nl_backing nl_v_27 = nl_b_6_10;
nl_b_6_10 = (nl_backing){0};
nl_allocation nl_b_6_11 = nl_v_27.allocation;
(void)nl_b_6_11;
nl_storage nl_b_6_12 = nl_v_27.raw;
(void)nl_b_6_12;
nl_v_27 = (nl_backing){0};
nl_allocation nl_v_28 = nl_b_6_11;
nl_b_6_11 = (nl_allocation){0};
nl_allocation nl_b_6_13 = nl_v_28;
(void)nl_b_6_13;
NL_NODE_BIND(6, 13, 4, &nl_b_6_13);
nl_storage nl_v_29 = nl_b_6_12;
nl_b_6_12 = (nl_storage){0};
nl_slot nl_v_30 = {nl_v_29.bytes,nl_v_29.length};
NL_HEAP_SLOT(nl_v_30.bytes,nl_v_30.length);
nl_slot nl_b_6_14 = nl_v_30;
(void)nl_b_6_14;
NL_NODE_BIND(6, 14, 4, &nl_b_6_14);
nl_domain nl_v_31 = {2};
NL_HEAP_DOMAIN(nl_v_31.token);
nl_domain nl_b_6_15 = nl_v_31;
(void)nl_b_6_15;
NL_NODE_BIND(6, 15, 4, &nl_b_6_15);
const nl_node * nl_v_32;
{ const nl_domain *nl_b_7_16 = &nl_b_6_15;
(void)nl_b_7_16;
nl_slot nl_v_33 = nl_b_6_14;
nl_b_6_14 = (nl_slot){0};
nl_node_option nl_v_34 = { 0, NULL };
uint8_t nl_v_35 = 212;
nl_node nl_v_36 = { nl_v_34, nl_v_35 };
const nl_domain * nl_v_37 = nl_b_7_16;
*(nl_node *)nl_v_33.bytes = nl_v_36;
const nl_node *nl_v_38 = (nl_node *)nl_v_33.bytes;
NL_HEAP_INITIALIZE(nl_v_38,nl_v_37->token);
(void)nl_v_37;
nl_v_32 = nl_v_38;
}
const nl_node * const nl_b_6_17 = nl_v_32;
(void)nl_b_6_17;
NL_NODE_BIND(6, 17, 2, &nl_b_6_17);
nl_node_option nl_v_39;
{ const nl_domain *nl_b_8_18 = &nl_b_2_7;
(void)nl_b_8_18;
const nl_node * nl_v_40 = nl_b_2_9;
const nl_domain * nl_v_41 = nl_b_8_18;
nl_node * nl_v_42 = (nl_node *)nl_v_40;
NL_HEAP_ROOT(nl_v_42,nl_v_41->token,1,8,11,3);
(void)nl_v_41;
nl_node * nl_b_8_19 = nl_v_42;
(void)nl_b_8_19;
NL_NODE_BIND(8, 19, 4, &nl_b_8_19);
nl_node * nl_v_43 = nl_b_8_19;
nl_node_option * nl_v_44 = &nl_v_43->f0;
NL_HEAP_FIELD(nl_v_43,nl_v_44,1,11,0,12,12,3);
const nl_node * nl_v_45 = nl_b_6_17;
nl_node_option nl_v_46 = { 1, nl_v_45 };
nl_node_option nl_v_47 = *nl_v_44;
*nl_v_44 = nl_v_46;
NL_HEAP_CHANGE(nl_v_44,&nl_v_47,1);
nl_v_39 = nl_v_47;
NL_HEAP_SCOPE_END(3);
}
nl_node_option nl_b_6_20 = nl_v_39;
(void)nl_b_6_20;
NL_NODE_BIND(6, 20, 3, &nl_b_6_20);
uint8_t nl_v_48;
{ const nl_domain *nl_b_9_21 = &nl_b_6_15;
(void)nl_b_9_21;
const nl_node * nl_v_49 = nl_b_6_17;
const nl_domain * nl_v_50 = nl_b_9_21;
const nl_node * nl_v_51 = nl_v_49;
NL_HEAP_ROOT(nl_v_51,nl_v_50->token,0,19,22,4);
(void)nl_v_50;
const nl_node * nl_b_9_22 = nl_v_51;
(void)nl_b_9_22;
NL_NODE_BIND(9, 22, 4, &nl_b_9_22);
const nl_node * nl_v_52 = nl_b_9_22;
const uint8_t * nl_v_53 = &nl_v_52->f1;
NL_HEAP_FIELD(nl_v_52,nl_v_53,0,11,1,24,24,4);
uint8_t nl_v_54 = *nl_v_53;
nl_v_48 = nl_v_54;
NL_HEAP_SCOPE_END(4);
}
uint8_t nl_b_6_23 = nl_v_48;
(void)nl_b_6_23;
NL_NODE_BIND(6, 23, 4, &nl_b_6_23);
nl_live_tail nl_v_55;
{ const nl_domain *nl_b_10_24 = &nl_b_2_7;
(void)nl_b_10_24;
const nl_node * nl_v_56 = nl_b_2_9;
const nl_domain * nl_v_57 = nl_b_10_24;
nl_node * nl_v_58 = (nl_node *)nl_v_56;
NL_HEAP_ROOT(nl_v_58,nl_v_57->token,1,8,11,5);
(void)nl_v_57;
nl_node * nl_b_10_25 = nl_v_58;
(void)nl_b_10_25;
NL_NODE_BIND(10, 25, 4, &nl_b_10_25);
nl_node * nl_v_59 = nl_b_10_25;
nl_node_option * nl_v_60 = &nl_v_59->f0;
NL_HEAP_FIELD(nl_v_59,nl_v_60,1,11,0,12,12,5);
const nl_node * nl_v_61 = nl_b_6_17;
nl_allocation nl_v_62 = nl_b_6_13;
nl_b_6_13 = (nl_allocation){0};
nl_domain nl_v_63 = nl_b_6_15;
nl_b_6_15 = (nl_domain){0};
NL_LIVE_SEND(nl_v_61,nl_v_62.handle,nl_v_63.token,&nl_b_6_13,&nl_b_6_15);
nl_live_tail nl_v_71 = nl_producer_13(nl_v_60,nl_v_61,nl_v_62,nl_v_63);
NL_LIVE_RECEIVE(nl_v_71,&nl_b_6_13,&nl_b_6_15);
nl_v_55 = nl_v_71;
NL_HEAP_SCOPE_END(5);
}
const nl_node * nl_b_6_31 = nl_v_55.ptr;
(void)nl_b_6_31;
nl_allocation nl_b_6_32 = nl_v_55.allocation;
(void)nl_b_6_32;
nl_domain nl_b_6_33 = nl_v_55.domain;
(void)nl_b_6_33;
nl_v_55 = (nl_live_tail){0};
const nl_node * nl_v_72 = nl_b_6_31;
nl_allocation nl_v_73 = nl_b_6_32;
nl_b_6_32 = (nl_allocation){0};
nl_domain nl_v_74 = nl_b_6_33;
nl_b_6_33 = (nl_domain){0};
NL_HEAP_HANDOFF(nl_v_72,nl_v_73.handle,nl_v_74.token,&nl_b_6_32,&nl_b_6_33);
nl_owner_11(nl_v_72,nl_v_73,nl_v_74);
NL_HEAP_RETURNED(&nl_b_6_32,&nl_b_6_33);
nl_slot nl_v_86;
{ const nl_domain *nl_b_15_42 = &nl_b_2_7;
(void)nl_b_15_42;
const nl_node * nl_v_87 = nl_b_2_9;
NL_HEAP_END(nl_v_87,nl_b_15_42->token);
nl_slot nl_v_88 = {(unsigned char *)nl_v_87,24};
nl_v_86 = nl_v_88;
}
nl_slot nl_b_6_43 = nl_v_86;
(void)nl_b_6_43;
NL_NODE_BIND(6, 43, 4, &nl_b_6_43);
nl_slot nl_v_89 = nl_b_6_43;
nl_b_6_43 = (nl_slot){0};
nl_storage nl_v_90 = {nl_v_89.bytes,nl_v_89.length};
NL_HEAP_RAW(nl_v_90.bytes,nl_v_90.length);
nl_storage nl_b_6_44 = nl_v_90;
(void)nl_b_6_44;
NL_NODE_BIND(6, 44, 4, &nl_b_6_44);
nl_domain nl_v_91 = nl_b_2_7;
nl_b_2_7 = (nl_domain){0};
NL_HEAP_FINALIZE(nl_v_91.token);
(void)nl_v_91;
nl_allocation nl_v_92 = nl_b_2_5;
nl_b_2_5 = (nl_allocation){0};
nl_storage nl_v_93 = nl_b_6_44;
nl_b_6_44 = (nl_storage){0};
NL_HEAP_RELEASE(nl_v_92.handle,nl_v_93.bytes,nl_v_93.length);
free(nl_v_92.handle);
nl_v_92.handle=NULL;
nl_v_93.bytes=NULL;
(void)nl_v_93;
uint8_t nl_v_94 = nl_b_6_23;
nl_v_16 = nl_v_94;
}
nl_v_1 = nl_v_16;
}
uint8_t nl_b_0_2=nl_v_1;
(void)nl_b_0_2;
NL_CURRENT_LOCAL(2,nl_b_0_2);
uint8_t nl_v_95=nl_b_0_2;
uint8_t nl_b_0_3=nl_v_95;
(void)nl_b_0_3;
NL_CURRENT_LOCAL(3,nl_b_0_3);
size_t nl_v_96=(size_t)UINTMAX_C(0);
uint8_t nl_v_97;
{
size_t nl_current_carry=nl_v_96;
for (;;) {
const size_t nl_current_phase=nl_current_carry;
NL_CURRENT_PHASE(nl_current_phase,&nl_b_0_1.f0,&nl_b_0_1.f1);
if (nl_current_phase < (size_t)UINTMAX_C(2)) {
size_t nl_v_98=nl_current_phase;
uint8_t nl_v_99=(nl_v_98 < (size_t)UINTMAX_C(1) ? 49 : 117);
uint8_t nl_b_16_6=nl_v_99;
(void)nl_b_16_6;
NL_CURRENT_LOCAL(6,nl_b_16_6);
uint8_t nl_v_100=nl_b_16_6;
uint8_t nl_v_101;
{
uint8_t *const nl_current_write=&nl_b_0_1.f0;
nl_v_101=*nl_current_write;
*nl_current_write=nl_v_100;
NL_MIXED_COPY_STORE(nl_current_write,nl_v_101,nl_v_100);
}
uint8_t nl_b_16_8=nl_v_101;
(void)nl_b_16_8;
NL_CURRENT_LOCAL(8,nl_b_16_8);
nl_current_carry=nl_current_phase+(size_t)UINTMAX_C(1);
continue;
}
uint8_t nl_v_102=nl_b_0_3;
uint8_t nl_v_103;
{
uint8_t *const nl_current_write=&nl_b_0_1.f0;
nl_v_103=*nl_current_write;
*nl_current_write=nl_v_102;
NL_MIXED_COPY_STORE(nl_current_write,nl_v_103,nl_v_102);
}
uint8_t nl_b_17_6=nl_v_103;
(void)nl_b_17_6;
NL_CURRENT_LOCAL(6,nl_b_17_6);
uint8_t nl_v_104=nl_b_17_6;
nl_v_97=nl_v_104;
NL_CURRENT_RESULT(nl_v_97);
break;
}
}
uint8_t nl_b_0_4=nl_v_97;
(void)nl_b_0_4;
NL_CURRENT_LOCAL(4,nl_b_0_4);
uint8_t nl_v_105=nl_b_0_1.f0;
NL_LOOP_READ(&nl_b_0_1.f0,nl_v_105);
uint8_t nl_b_0_5=nl_v_105;
(void)nl_b_0_5;
NL_CURRENT_LOCAL(5,nl_b_0_5);
uint8_t nl_v_106=nl_b_0_1.f1;
NL_LOOP_READ(&nl_b_0_1.f1,nl_v_106);
uint8_t nl_b_0_6=nl_v_106;
(void)nl_b_0_6;
NL_CURRENT_LOCAL(6,nl_b_0_6);
uint8_t nl_v_107=nl_b_0_4;
uint8_t nl_b_0_7=nl_v_107;
(void)nl_b_0_7;
NL_CURRENT_LOCAL(7,nl_b_0_7);
NL_HEAP_FINISH();
return 0;
}
