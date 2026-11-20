fn willow_tail(seed: usize) -> usize {
    let quotient = seed / usize(7);
    let remembered = quotient;
    let narrow = quotient / usize(3);
    narrow;
    remembered
}

fn willow_caller() -> usize {
    let original = usize(1019);
    let received = willow_tail(original);
    let retained = received;
    let local_piece = received / usize(11);
    let again = willow_tail(retained);
    received;
    local_piece;
    again
}
