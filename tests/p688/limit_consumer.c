#include "newlang/experimental_c17_u8_export_v0.h"
int main(void)
{
    uint8_t sample = newlang_c17_u8_export_v0();
    if (sample != 53)
        return 1;
    return 0;
}
