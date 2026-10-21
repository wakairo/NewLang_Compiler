#include "newlang/experimental_c17_u8_export_v0.h"
#include <stdint.h>

int main(void)
{
    uint8_t observed = newlang_c17_u8_export_v0();
    return observed == UINT8_C(29) ? 0 : 21;
}
