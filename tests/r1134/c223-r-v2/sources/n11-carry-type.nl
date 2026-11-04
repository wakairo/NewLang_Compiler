fn main() -> unit {
    loop (phase = usize(2)) {
        if (phase < usize(7)) {
            continue(u8(3));
        } else {
            break unit;
        }
    }
}
