fn main() -> unit {
    loop (phase = usize(1)) {
        if (phase < usize(3)) {
            continue(phase - usize(1));
        } else {
            break unit;
        }
    }
}
