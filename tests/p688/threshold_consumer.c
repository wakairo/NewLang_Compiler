#include "newlang/experimental_c17_u8_export_v0.h"
int main(void)
{
    const uint8_t result = newlang_c17_u8_export_v0();
    return result == 17 ? 0 : 1;
}
