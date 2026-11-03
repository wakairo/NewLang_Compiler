fn r1120_wrong_arm(observed: usize) -> u8 {
    if (observed < usize(613)) {
        u8(71)
    } else {
        usize(71)
    }
}

fn main() -> unit {
    unit
}
