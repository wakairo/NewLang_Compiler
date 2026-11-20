fn divide_zero(original: usize) -> usize {
    let operand = original;
    operand / usize(0)
}

fn main() -> unit {
    let quantity = divide_zero(usize(26));
    unit
}
