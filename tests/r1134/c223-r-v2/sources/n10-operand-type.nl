fn main() -> unit {
    loop (phase = usize(2)) {
        if (phase < usize(7)) {
            continue(phase + u8(1));
        } else {
            break unit;
        }
    }
}
