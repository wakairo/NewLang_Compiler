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
int main(void) {
nl_node_option nl_v_1 = { 0, NULL };
uint8_t nl_v_2 = 29;
nl_node nl_v_3 = { nl_v_1, nl_v_2 };
nl_node nl_b_0_1 = nl_v_3;
(void)nl_b_0_1;
NL_NODE_BIND(0, 1, 1, &nl_b_0_1);
const nl_node * nl_v_4;
{ const nl_node *nl_ref_5 = &nl_b_0_1;
nl_v_4 = nl_ref_5; }
const nl_node * const nl_b_0_3 = nl_v_4;
(void)nl_b_0_3;
NL_NODE_BIND(0, 3, 2, &nl_b_0_3);
const nl_node * nl_v_6 = nl_b_0_3;
nl_node_option nl_v_7 = { 1, nl_v_6 };
uint8_t nl_v_8 = 61;
nl_node nl_v_9 = { nl_v_7, nl_v_8 };
nl_node nl_b_0_4 = nl_v_9;
(void)nl_b_0_4;
NL_NODE_BIND(0, 4, 1, &nl_b_0_4);
nl_node_option nl_v_10 = nl_b_0_4.f0;
const nl_node_option nl_b_0_5 = nl_v_10;
(void)nl_b_0_5;
NL_NODE_BIND(0, 5, 3, &nl_b_0_5);
nl_node_option nl_v_11 = nl_b_0_5;
(void)nl_v_11;
NL_NODE_FINISH();
return 0;
}
