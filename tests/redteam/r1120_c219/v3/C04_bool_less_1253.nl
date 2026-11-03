fn main() -> unit {
    let sample_0 = r1120_v2_bool_less_1253(usize(0));
    let sample_1 = r1120_v2_bool_less_1253(usize(997));
    let sample_2 = r1120_v2_bool_less_1253(usize(1252));
    let sample_3 = r1120_v2_bool_less_1253(usize(1253));
    let sample_4 = r1120_v2_bool_less_1253(usize(1254));
    unit
}

fn r1120_v2_bool_less_1253(observed: usize) -> u8 {
    let copied = observed;
    let threshold = usize(1253);
    let below = copied < threshold;
    if (below) {
        u8(152)
    } else {
        u8(219)
    }
}
