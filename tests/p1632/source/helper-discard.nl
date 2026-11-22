fn fold_groups(payload: usize) -> usize {
    payload / usize(9)
}

fn band(permit: bool, payload: usize) -> usize {
    permit;
    let policy = permit;
    let amount = if (policy) {
        let levels = payload / usize(8);
        levels
    } else {
        let excess = payload % usize(13);
        excess
    };
    return amount;
}

fn main() -> unit {
    let count = usize(153);
    let first = band(count < usize(200), fold_groups(count));
    let later = band(first == usize(3), fold_groups(usize(288)));
    first;
    later;
    return unit;
}
