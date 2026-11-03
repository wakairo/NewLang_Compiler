fn effectful(sample: usize) -> u8 {
    let cell = u8(19);
    let discarded = loan_write(cell) { |slot| replace(slot, u8(88)) };
    let answer = if (sample == usize(257)) { u8(134) } else { u8(211) };
    answer
}
fn main() -> unit {
    let result = effectful(usize(257));
    return unit;
}
