fn main() -> unit {
    let answer = ratio(usize(4), usize(29));
}

fn ratio(n: usize, d: usize) -> usize {
    if (d != usize(0)) {
        n / d
    } else {
        usize(31)
    }
}
