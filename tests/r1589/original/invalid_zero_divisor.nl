fn select(amount: usize) -> usize {
    let joined = if (amount < usize(11)) {
        usize(22)
    } else {
        usize(35)
    };
    let copied = joined;
    copied / usize(0)
}

fn main() -> unit {
    let answer = select(usize(7));
    unit
}
