fn main() -> unit {
    let sample_0 = r1120_v2_equal_913(usize(0));
    let sample_1 = r1120_v2_equal_913(usize(612));
    let sample_2 = r1120_v2_equal_913(usize(613));
    let sample_3 = r1120_v2_equal_913(usize(657));
    let sample_4 = r1120_v2_equal_913(usize(768));
    let sample_5 = r1120_v2_equal_913(usize(912));
    let sample_6 = r1120_v2_equal_913(usize(913));
    let sample_7 = r1120_v2_equal_913(usize(914));
    unit
}

fn r1120_v2_equal_913(observed: usize) -> u8 {
    let exactly = observed == usize(913);
    if (exactly) {
        u8(47)
    } else {
        u8(219)
    }
}
