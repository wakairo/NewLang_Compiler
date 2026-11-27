fn sensor_octet(level: usize) -> u8 {
    if (level < usize(256)) {
        u8(level)
    } else {
        u8(217)
    }
}

fn main() -> unit {
    let level = usize(255);
    let sample = sensor_octet(level);
    let reported = sample;
    return unit;
}
