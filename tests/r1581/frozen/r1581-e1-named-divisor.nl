fn named(input: usize) -> usize {
    let divisor = usize(7);
    let quotient = input / divisor;
    let remainder = quotient % usize(5);
    remainder
}

fn main() -> unit {
    let result = named(usize(173));
    unit
}
