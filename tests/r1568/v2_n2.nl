fn remainder_zero(original: usize) -> usize {
    let first_copy = original;
    let operand = first_copy;
    let quantity = operand % usize(0);
    return quantity;
}

fn main() -> unit {
    let quantity = remainder_zero(usize(41));
    unit
}
