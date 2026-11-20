fn divide_negative_literal(original: usize) -> usize {
    let operand = original;
    operand / usize(-1)
}

fn main() -> unit {
    let quantity = divide_negative_literal(usize(26));
    unit
}
