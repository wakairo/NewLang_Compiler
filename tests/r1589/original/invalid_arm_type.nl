fn choose(amount: usize) -> usize {
    if (amount == usize(0)) {
        usize(5)
    } else {
        u8(5)
    }
}

fn main() -> unit {
    let answer = choose(usize(1));
    unit
}
