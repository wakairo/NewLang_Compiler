fn r1120_bool_equal(observed: usize) -> bool {
    observed == usize(1253)
}

fn r1120_bool_less(observed: usize) -> bool {
    observed < usize(1253)
}

fn main() -> unit {
    let sample_0 = r1120_bool_equal(usize(0));
    let sample_1 = r1120_bool_less(usize(0));
    let sample_2 = r1120_bool_equal(usize(997));
    let sample_3 = r1120_bool_less(usize(997));
    let sample_4 = r1120_bool_equal(usize(1252));
    let sample_5 = r1120_bool_less(usize(1252));
    let sample_6 = r1120_bool_equal(usize(1253));
    let sample_7 = r1120_bool_less(usize(1253));
    let sample_8 = r1120_bool_equal(usize(1254));
    let sample_9 = r1120_bool_less(usize(1254));
    unit
}
