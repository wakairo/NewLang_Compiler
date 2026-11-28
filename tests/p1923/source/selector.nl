fn sample_slots(load: usize) -> u8 {
    if (load < usize(91)) { u8(53) } else { u8(211) }
}

fn slots_count(octet: u8) -> usize {
    let widened = usize(octet);
    widened
}

fn pick_label(count: usize) -> u8 {
    if (count < usize(102)) { u8(89) } else { u8(17) }
}

fn main() -> unit {
    let load = usize(91);
    let octet = sample_slots(load);
    let count = slots_count(octet);
    let chosen = pick_label(count);
    let saved = chosen;
    return unit;
}
