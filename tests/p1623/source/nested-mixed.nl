fn split(seed: usize) -> usize {
    seed / usize(4)
}

fn choose(clip: bool, quantity: usize) -> usize {
    let source = quantity;
    let selected = if (clip) {
        source
    } else {
        let remainder = source % usize(7);
        remainder
    };
    selected
}

fn downstream(first: usize, ignored: bool) -> usize {
    let result = first;
    result;
    return result;
}

fn main() -> unit {
    let original = usize(62);
    let intermediate = choose(original < usize(11), split(original));
    let sense = intermediate < usize(4);
    let early = choose(sense, usize(23));
    let late = downstream(choose(original < usize(70), split(original)), original == usize(62));
    let audit = late % usize(8);
    intermediate;
    early;
    late;
    audit;
    unit
}
