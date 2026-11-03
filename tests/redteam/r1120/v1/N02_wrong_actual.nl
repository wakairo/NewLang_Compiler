fn r1120_needs_usize(item: usize) -> u8 {
    u8(31)
}

fn r1120_wrong_actual(item: u8) -> u8 {
    r1120_needs_usize(item)
}
