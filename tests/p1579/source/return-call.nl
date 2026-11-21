fn main() -> unit {
    let initial = usize(143);
    let first = recover(initial);
    let extra = recover(usize(95));
    let summed = first + extra;
    unit
}

fn recover(amount: usize) -> usize {
    let held = amount;
    return sieve(held);
}

fn sieve(start: usize) -> usize {
    let rest = start % usize(31);
    rest / usize(4)
}
