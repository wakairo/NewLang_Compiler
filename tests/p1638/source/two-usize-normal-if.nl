fn apportion_sample(total: usize, cutoff: usize) -> usize {
    let selected = if (total < cutoff) {
        let quotient = total / usize(7);
        quotient
    } else {
        let residue = total % usize(9);
        residue
    };
    let carried = selected;
    carried
}

fn main() -> unit {
    let total = usize(86);
    let early = usize(100);
    let ordinary = usize(30);
    let first = apportion_sample(total, early);
    let second = apportion_sample(total, ordinary);
    first;
    second;
}
