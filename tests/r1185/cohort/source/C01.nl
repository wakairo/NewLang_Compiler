fn ordinary_pick(tick: usize) -> u8 {
    if (tick < usize(4)) {
        u8(29)
    } else {
        u8(162)
    }
}

fn main() -> unit {
    let phase = usize(5);
    let normal = ordinary_pick(phase);
    let normal_copy = normal;
    let phase_after = phase;
    unit
}
