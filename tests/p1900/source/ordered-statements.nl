fn choose_tag(seed: usize) -> u8 {
    if (seed < usize(64)) { u8(11) } else { u8(239) }
}

fn retain_tag(tag: u8) -> unit {
    let saved = tag;
    saved;
    return unit;
}

fn main() -> unit {
    let seed = usize(17);
    let available = choose_tag(seed);
    retain_tag(available);
    let copied_first = available;
    let second_seed = usize(68);
    let later = choose_tag(second_seed);
    retain_tag(later);
    let copied_second = later;
    return unit;
}
