fn sensor_octet(level: usize) -> u8 {
    if (level < usize(256)) {
        u8(217)
    } else {
        u8(level)
    }
}

fn main() -> unit {
    let level = usize(256);
    let sample = sensor_octet(level);
    let reported = sample;
    return unit;
}
