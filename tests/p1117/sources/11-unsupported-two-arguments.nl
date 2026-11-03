fn select_two(sample: usize, limit: usize) -> u8 {
    let answer = if (sample < limit) { u8(134) } else { u8(211) };
    answer
}
fn main() -> unit {
    let result = select_two(usize(1), usize(257));
    return unit;
}
