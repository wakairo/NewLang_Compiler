fn revisit(quantity: usize) -> usize {
    let reduced = quantity / usize(3);
    revisit(reduced)
}

fn main() -> unit {
    let report = revisit(usize(61));
    unit
}
