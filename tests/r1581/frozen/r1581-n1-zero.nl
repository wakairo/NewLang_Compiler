fn clean(input: usize) -> usize {
    let remainder = input % usize(11);
    let quotient = remainder / usize(2);
    quotient
}

fn broken(input: usize) -> usize {
    let middle = clean(input);
    let result = middle / usize(0);
    result
}

fn main() -> unit {
    let result = broken(usize(73));
    unit
}
