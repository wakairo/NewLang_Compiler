fn measure_quotient(amount: usize) -> usize {
    let quotient = amount / usize(1);
    let forwarded = quotient;
    return forwarded;
}

fn main() -> unit {
    let first_result = measure_quotient(usize(0));
    let second_result = measure_quotient(usize(7));
    let first_copy = first_result;
    let second_copy = second_result;
    unit
}
