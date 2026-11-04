fn main() -> unit {
    loop (phase = usize(9)) {
        if (phase < usize(9)) {
            break unit;
        } else {
            continue(phase + usize(1));
        }
    }
}
