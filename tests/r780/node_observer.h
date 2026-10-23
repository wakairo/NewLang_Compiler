/* R #780 independently owned, passive node-binding observation. */
#include <stddef.h>
static void r780_node_bind(size_t, size_t, size_t, const void *);
#define NL_NODE_BIND(f, s, k, p) r780_node_bind(f, s, k, p)
#define NL_NODE_REPLACE(...) ((void)0)
#define NL_NODE_ARM(...) ((void)0)
#define NL_NODE_RELOAN(...) ((void)0)
#define NL_NODE_FINISH() ((void)0)
