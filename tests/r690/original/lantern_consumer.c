#include "newlang/experimental_c17_u8_export_v0.h"
#include <stdint.h>

static int result_is_expected(uint8_t value) {
    if (value != UINT8_C(61)) {
        return 31;
    }
    return 0;
}

int main(void) {
    const uint8_t observed = newlang_c17_u8_export_v0();
    return result_is_expected(observed);
}
