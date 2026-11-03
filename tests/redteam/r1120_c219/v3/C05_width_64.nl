fn main() -> unit {
    let sample_0 = r1120_v2_width_64(usize(4294967295));
    let sample_1 = r1120_v2_width_64(usize(4294967296));
    let sample_2 = r1120_v2_width_64(usize(18446744073709551615));
    unit
}

fn r1120_v2_width_64(observed: usize) -> u8 {
    let below = observed < usize(4294967296);
    if (below) {
        u8(152)
    } else {
        u8(219)
    }
}
