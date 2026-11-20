fn crates_for(pieces: usize) -> usize {
    let crates = pieces / usize(5);
    let packed = crates;
    return packed;
}

fn loads_for(crates: usize) -> usize {
    crates / usize(3)
}

fn main() -> unit {
    let first_crates = crates_for(usize(173));
    let for_shipping = first_crates;
    let first_loads = loads_for(for_shipping);
    let reserve_crates = crates_for(first_crates);
    let reserve_loads = loads_for(reserve_crates);
    let original_crates = first_crates;
    let loads_with_reserve = first_loads + reserve_loads;
    unit
}
