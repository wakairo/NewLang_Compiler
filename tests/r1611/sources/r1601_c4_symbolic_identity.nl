fn identity(n: usize) -> usize {
    n
}

fn relay(n: usize) -> usize {
    identity(n / usize(1))
}

fn main() -> unit {
    let zero_result = relay(usize(0) + usize(0));
    let nonzero_result = relay(usize(5) + usize(2));
    unit
}
