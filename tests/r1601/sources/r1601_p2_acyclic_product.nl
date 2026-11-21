fn residue(n: usize) -> usize {
    n % usize(6)
}

fn reduce(n: usize) -> usize {
    residue(n / usize(3))
}

fn main() -> unit {
    let result = reduce(usize(8) * usize(7));
    unit
}
