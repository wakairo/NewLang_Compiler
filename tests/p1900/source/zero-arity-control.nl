fn choose_tag(seed: usize) -> u8 {
    if (seed < usize(64)) { u8(11) } else { u8(239) }
}

fn tick() -> unit {
    return unit;
}

fn main() -> unit {
    let seed = usize(17);
    let available = choose_tag(seed);
    tick();
    let observed = available;
    return unit;
}
