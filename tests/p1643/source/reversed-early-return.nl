fn apportion_sample(take_early: bool, total: usize) -> usize {
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
    let first = apportion_sample(early, total);
    let second = apportion_sample(ordinary, total);
    first;
    second;
}
