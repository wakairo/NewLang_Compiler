fn read_quota(seed: usize) -> u8 {
    if (seed < usize(64)) { u8(0) } else { u8(255) }
}

fn widen_quota(octet: u8) -> usize {
    usize(octet)
}

fn choose_priority(quota: usize) -> u8 {
    if (quota < usize(128)) { u8(7) } else { u8(9) }
}

fn main() -> unit {
    let seed = usize(0);
    let octet = read_quota(seed);
    let quota = widen_quota(octet);
    let priority = choose_priority(quota);
    let observed = priority;
    return unit;
}
