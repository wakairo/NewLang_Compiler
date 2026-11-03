fn classify_wide(sample: usize) -> u8 {
    let copy = sample;
    let answer = if (copy == usize(257)) { u8(134) } else { u8(211) };
    answer
}
fn main() -> unit {
    let truncated = classify_wide(usize(1));
    let exact = classify_wide(usize(257));
    let other = classify_wide(usize(258));
    return unit;
}
