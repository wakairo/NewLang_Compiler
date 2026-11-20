fn remainder_copies(original: usize) -> usize {
    let first_copy = original;
    let operand = first_copy;
    let quantity = operand % usize(7);
    let result_copy = quantity;
    return result_copy;
}

fn main() -> unit {
    let actual = usize(41);
    let first_result = remainder_copies(actual);
    let result_copy = first_result;
    let forwarded_result = remainder_copies(result_copy);
    let reused_result = remainder_copies(first_result);
    return unit;
}
