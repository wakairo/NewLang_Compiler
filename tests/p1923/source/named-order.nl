fn sample_slots(load: usize) -> u8 {
    if (load < usize(91)) { u8(53) } else { u8(211) }
}

fn slots_count(octet: u8) -> usize {
    let widened = usize(octet);
    widened
}

fn main() -> unit {
    let load = usize(91);
    let octet = sample_slots(load);
    let count = slots_count(octet);
    let current = count;
    let above = usize(102) < current;
    let decision = above;
    let chosen = if (decision) { u8(17) } else { u8(89) };
    let saved = chosen;
    return unit;
}
