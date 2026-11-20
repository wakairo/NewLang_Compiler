fn collect(input: usize) -> usize {
    let base = input / usize(7);
    let divisor = usize(6);
    base % divisor
}

fn main() -> unit {
    let result = collect(usize(81));
    unit
}
