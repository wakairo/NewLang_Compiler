fn ratio(n: usize, d: usize) -> usize {
    if (d != usize(0)) {
        n / d
    } else {
        usize(31)
    }
}

fn main() -> unit {
    let answer = ratio(usize(29), usize(4));
}
