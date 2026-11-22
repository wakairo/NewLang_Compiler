fn apportion_sample(total: usize, take_early: bool) -> usize {
    let selected = if (take_early) {
        let quotient = total / usize(7);
        return quotient;
    } else {
        let residue = total % usize(9);
        residue
    };
    let carried = selected;
    carried
}

fn main() -> unit {
    let total = usize(86);
    let early = total < usize(100);
    let ordinary = total < usize(30);
    let first = apportion_sample(total, early);
    let second = apportion_sample(total, ordinary);
    first;
    second;
}
