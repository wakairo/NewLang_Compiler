struct Barometer { upper: u8, lower: u8 }
fn balance(write_ref: ref<write, u8>, supply: u8) -> unit {
    if (usize(22) == usize(77)) {
        return unit;
    } else {
        let retired = replace(write_ref, supply);
        unit
    };
    unit
}
fn main() -> unit {
    let sample = Barometer { lower: u8(92), upper: u8(11) };
    let original = sample@lower;
    let side = sample@upper;
    loan_write(sample@lower) { |borrowed|
        balance(borrowed, u8(48))
    };
    let joined_field = sample@lower;
    let original_still = original;
    let side_again = sample@upper;
    let side_snapshot = side;
    unit
}
