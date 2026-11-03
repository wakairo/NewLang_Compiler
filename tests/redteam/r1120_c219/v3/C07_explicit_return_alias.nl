fn main() -> unit {
    let sample_0 = r1120_v3_return_alias(usize(657));
    let sample_1 = r1120_v3_return_alias(usize(913));
    let sample_2 = r1120_v3_return_alias(usize(914));
    unit
}

fn r1120_v3_return_alias(observed: usize) -> u8 {
    let copied = observed;
    let same = copied == usize(913);
    let condition = same;
    if (condition) {
        let selected = u8(47);
        return selected;
    } else {
        let selected = u8(219);
        return selected;
    }
}
