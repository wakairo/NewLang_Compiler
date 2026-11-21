fn sift(amount: usize) -> usize {
    amount % usize(11)
}

fn main() -> unit {
    let first_argument = quarter(usize(146));
    let first = sift(first_argument);
    let second_argument = sift(usize(146));
    let second = quarter(second_argument);
    let repeat_argument = quarter(usize(146));
    let repeat = sift(repeat_argument);
    let copied = first;
    let copied_argument = sift(copied);
    let copied_round = quarter(copied_argument);
    unit
}

fn quarter(amount: usize) -> usize {
    amount / usize(4)
}
