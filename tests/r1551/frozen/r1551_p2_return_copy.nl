fn copper_return(seed: usize) -> usize {
    let share = seed / usize(5);
    let retained = share;
    let discarded = share / usize(2);
    discarded;
    return retained;
}

fn copper_caller() -> usize {
    let source = usize(997);
    let returned = copper_return(source);
    let mirror = returned;
    let local_piece = returned / usize(3);
    let reentered = copper_return(mirror);
    returned;
    local_piece;
    return reentered;
}
