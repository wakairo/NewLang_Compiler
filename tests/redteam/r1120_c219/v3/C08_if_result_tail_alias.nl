fn main() -> unit {
    let sample_0 = r1120_v3_tail_alias(usize(612));
    let sample_1 = r1120_v3_tail_alias(usize(613));
    let sample_2 = r1120_v3_tail_alias(usize(768));
    let sample_3 = r1120_v3_tail_alias(usize(913));
    unit
}

fn r1120_v3_tail_alias(observed: usize) -> u8 {
    let selected = if (observed < usize(613)) {
        let value = u8(152);
        value
    } else {
        let value = u8(219);
        value
    };
    let result_alias = selected;
    result_alias
}
