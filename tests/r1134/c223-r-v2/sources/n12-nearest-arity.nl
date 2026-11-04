fn main() -> unit {
    loop (phase = usize(2)) {
        if (phase < usize(7)) {
            continue(phase, phase);
        } else {
            break unit;
        }
    }
}
