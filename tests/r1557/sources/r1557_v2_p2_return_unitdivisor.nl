fn single_bucket(amount: usize) -> usize {
    let residual = amount % usize(1);
    let forwarded = residual;
    return forwarded;
}

fn main() -> unit {
    let first_result = single_bucket(usize(0));
    let second_result = single_bucket(usize(65535));
    let first_copy = first_result;
    let second_copy = second_result;
    unit
}
