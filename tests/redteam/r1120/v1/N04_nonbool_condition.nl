fn r1120_nonbool_condition(observed: usize) -> u8 {
    if (observed) {
        u8(13)
    } else {
        u8(29)
    }
}
