fn sift(amount: usize) -> usize {
    amount % usize(11)
}

fn main() -> unit {
    let first = sift(quarter(usize(146)));
    let second = quarter(sift(usize(146)));
    let repeat = sift(quarter(usize(146)));
    let copied = first;
    let copied_round = quarter(sift(copied));
    unit
}

fn quarter(amount: usize) -> usize {
    amount / usize(4)
}
