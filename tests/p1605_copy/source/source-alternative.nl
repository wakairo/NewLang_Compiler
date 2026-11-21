fn dispatch(input: usize) -> usize {
    let original = input;
    let selected = if (original < usize(45)) {
        let portions = original / usize(5);
        portions
    } else {
        let leftover = original % usize(12);
        leftover
    };
    let kept = selected;
    kept
}

fn main() -> unit {
    let low = dispatch(usize(37));
    let high = dispatch(usize(74));
    let audit = high % usize(3);
    return unit;
}
