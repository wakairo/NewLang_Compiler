fn main() -> unit {
    let order = usize(173);
    let first_batches = batch(order);
    let left = overflow(first_batches);
    let second_batches = batch(usize(446));
    let remainder = overflow(second_batches);
    let stock = rack(remainder);
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
