fn fold(units: usize) -> usize {
    let folded = units / usize(6);
    folded
}

fn main() -> unit {
    let inventory = usize(61);
    let qty = fold(inventory);
    let room = qty + usize(2);
    unit
}
