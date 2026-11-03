fn main() -> unit {
    let sample_0 = r1120_v2_equal_913_max64(usize(18446744073709551615));
    let sample_1 = r1120_v2_less_613_max64(usize(18446744073709551615));
    let sample_2 = r1120_v2_bool_equal_1253_max64(usize(18446744073709551615));
    let sample_3 = r1120_v2_bool_less_1253_max64(usize(18446744073709551615));
    unit
}

fn r1120_v2_equal_913_max64(observed: usize) -> u8 {
    let exactly = observed == usize(913);
    if (exactly) {
        u8(47)
    } else {
        u8(219)
    }
}

fn r1120_v2_less_613_max64(observed: usize) -> u8 {
    if (observed < usize(613)) {
        u8(152)
    } else {
        u8(219)
    }
}

fn r1120_v2_bool_equal_1253_max64(observed: usize) -> u8 {
    let compared = usize(1253) == observed;
    if (compared) {
        u8(47)
    } else {
        u8(219)
    }
}

fn r1120_v2_bool_less_1253_max64(observed: usize) -> u8 {
    let copied = observed;
    let threshold = usize(1253);
    let below = copied < threshold;
    if (below) {
        u8(152)
    } else {
        u8(219)
    }
}
