fn main() -> unit {
    let n = usize(47);
    let d = usize(6);
    let first = modulo(n, d);
    let second = modulo(d, n);
    let kept = n / usize(1);
}

fn modulo(n: usize, d: usize) -> usize {
    if (d != usize(0)) {
        n % d
    } else {
        usize(37)
    }
}
