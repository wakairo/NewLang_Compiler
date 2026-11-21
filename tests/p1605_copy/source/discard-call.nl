fn portions(input: usize) -> usize {
    input / usize(5)
}

fn main() -> unit {
    let number = usize(37);
    portions(number);
    let remainder = number % usize(12);
    unit
}
