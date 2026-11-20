fn divide_copy(original: usize) -> usize {
    let operand = original;
    operand / usize(4)
}

fn main() -> unit {
    let actual = usize(26);
    let first_result = divide_copy(actual);
    let equal_result = divide_copy(usize(27));
    let reused_actual_result = divide_copy(actual);
    let forwarded_result = divide_copy(first_result);
    unit
}
