fn sensor_octet(level: usize) -> u8 {
    let accepted = u8(level);
    let forwarded = accepted;
    forwarded
}

fn main() -> unit {
    let level = usize(255);
    let sample = sensor_octet(level);
    let reported = sample;
    return unit;
}
