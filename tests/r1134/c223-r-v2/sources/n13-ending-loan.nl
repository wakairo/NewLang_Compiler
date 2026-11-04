fn main() -> unit {
    let cell = u8(68);
    let escaped = loan_write(cell) { |slot|
        loop (phase = usize(3)) {
            if (phase < usize(1)) {
                break slot;
            } else {
                continue(phase - usize(1));
            }
        }
    };
    unit
}
