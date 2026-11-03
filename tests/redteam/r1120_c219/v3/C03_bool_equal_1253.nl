fn main() -> unit {
    let sample_0 = r1120_v2_bool_equal_1253(usize(0));
    let sample_1 = r1120_v2_bool_equal_1253(usize(997));
    let sample_2 = r1120_v2_bool_equal_1253(usize(1252));
    let sample_3 = r1120_v2_bool_equal_1253(usize(1253));
    let sample_4 = r1120_v2_bool_equal_1253(usize(1254));
    unit
}

fn r1120_v2_bool_equal_1253(observed: usize) -> u8 {
    let compared = usize(1253) == observed;
    if (compared) {
        u8(47)
    } else {
        u8(219)
    }
}
