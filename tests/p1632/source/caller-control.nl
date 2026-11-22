fn packed(weight: usize, compact: bool) -> usize {
    let in_bin = if (compact) {
        let full = weight / usize(8);
        full
    } else {
        let excess = weight % usize(13);
        excess
    };
    in_bin
}

fn main() -> unit {
    let weight = usize(101);
    let permit = weight < usize(120);
    let packed_count = packed(weight, permit);
    let rule = packed_count == usize(11);
    let hold = rule;
    let leftovers = packed(weight, hold);
    let audit = leftovers % usize(3);
    audit;
}
