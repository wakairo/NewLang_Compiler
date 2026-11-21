fn dispatch(input: usize) -> usize {
    let original = input;
    original;
    let selected = if (original < usize(45)) {
        let portions = original / usize(5);
        portions;
        portions
    } else {
        let leftover = original % usize(12);
        leftover;
        leftover
    };
    selected;
    let kept = selected;
    kept;
    kept
}

fn main() -> unit {
    let low = dispatch(usize(37));
    low;
    let high = dispatch(usize(74));
    high;
    let audit = high % usize(3);
    audit;
    return unit;
}
