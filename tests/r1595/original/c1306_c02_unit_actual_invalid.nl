fn sift(amount: usize) -> usize {
    amount % usize(11)
}

fn main() -> unit {
    let bad = sift(quarter(unit));
    unit
}

fn quarter(amount: usize) -> usize {
    amount / usize(4)
}
