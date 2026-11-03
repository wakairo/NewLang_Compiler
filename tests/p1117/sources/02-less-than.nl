fn classify_sample(measurement: usize) -> u8 {
    let answer = if (measurement < usize(24)) { u8(113) } else { u8(247) };
    answer
}
fn main() -> unit {
    let below = classify_sample(usize(23));
    let equal = classify_sample(usize(24));
    let above = classify_sample(usize(26));
    return unit;
}
