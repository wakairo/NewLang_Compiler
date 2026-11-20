fn divide_copy(original: usize) -> usize {
    let operand = original;
    operand / usize(4)
}

fn main() -> unit {
    let quantity = divide_copy(usize(26));
    let final_quantity = quantity / usize(2);
    unit
}
