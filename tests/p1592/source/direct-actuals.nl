fn main() -> unit {
    let order = usize(173);
    let left = overflow(batch(order));
    let stock = rack(overflow(batch(usize(446))));
    let copied = stock;
    let parity = copied % usize(2);
    unit
}

fn rack(units: usize) -> usize {
    units / usize(3)
}

fn overflow(units: usize) -> usize {
    units % usize(13)
}

fn batch(units: usize) -> usize {
    units / usize(6)
}
