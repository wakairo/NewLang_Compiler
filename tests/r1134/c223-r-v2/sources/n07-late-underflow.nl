fn main() -> unit {
    loop (phase = usize(8)) {
        if (phase < usize(2)) {
            break unit;
        } else {
            continue(phase - usize(3));
        }
    }
}
