fn estimate_pallets(loaded_crates: usize) -> usize {
    loaded_crates / usize(23)
}

fn main() -> unit {
    let counted = estimate_pallets(usize(1601));
    let copy_count = counted;
    let extra = copy_count + usize(1);
    unit
}
