fn knot(value: usize) -> usize {
    let bounded = value % usize(97);
    let folded = cut(cut(bounded));
    let residue = folded % usize(11);
    let output = trim(residue);
    return output;
}

fn trim(value: usize) -> usize {
    let residue = value % usize(13);
    let quotient = residue / usize(2);
    quotient
}

fn cut(value: usize) -> usize {
    let quotient = value / usize(2);
    let residue = quotient % usize(29);
    return residue;
}

fn main() -> unit {
    let seed = usize(191);
    let top = knot(seed);
    let nested = trim(cut(usize(95)));
    let same = trim(usize(5));
    let repeat = knot(usize(153));
    let sum = top + nested;
    let delta = sum - same;
    let final = delta + repeat;
    return unit;
}
