fn main() -> unit {
    let bad = sift(splitzero(usize(146)));
    unit
}

fn splitzero(amount: usize) -> usize {
    amount / usize(0)
}

fn sift(amount: usize) -> usize {
    amount % usize(11)
}
