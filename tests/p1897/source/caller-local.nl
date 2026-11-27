fn read_quota(seed: usize) -> u8 {
    if (seed < usize(64)) { u8(0) } else { u8(255) }
}

fn main() -> unit {
    let seed = usize(64);
    let octet = read_quota(seed);
    let observed = usize(octet);
    return unit;
}
