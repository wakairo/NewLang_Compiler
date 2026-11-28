fn sensor_octet(level: usize) -> u8 {
    if (level < usize(32)) {
        let accepted = u8(level);
        let forwarded = accepted;
        let reported = forwarded;
        reported
    } else {
        u8(201)
    }
}

fn main() -> unit {
    let level = usize(31);
    let sample = sensor_octet(level);
    let reported = sample;
    return unit;
}
