fn main() -> unit {
    loop (phase = usize(1)) {
        if (phase < usize(6)) {
            continue(usize(2) + phase);
        } else {
            break unit;
        }
    }
}
