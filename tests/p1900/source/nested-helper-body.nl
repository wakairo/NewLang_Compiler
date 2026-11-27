fn choose_tag(seed: usize) -> u8 {
    if (seed < usize(64)) { u8(11) } else { u8(239) }
}

fn copy_tag(tag: u8) -> u8 {
    let copy = tag;
    copy
}

fn retain_tag(tag: u8) -> unit {
    let saved = copy_tag(tag);
    saved;
    return unit;
}

fn main() -> unit {
    let seed = usize(17);
    let available = choose_tag(seed);
    retain_tag(available);
    let observed = available;
    return unit;
}
