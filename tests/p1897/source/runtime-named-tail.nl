fn read_quota(seed: usize) -> u8 {
    if (seed < usize(64)) { u8(0) } else { u8(255) }
}

fn widen_quota(octet: u8) -> usize {
    let count = usize(octet);
    count
}

fn main() -> unit {
    let seed = usize(0);
    let octet = read_quota(seed);
    let quota = widen_quota(octet);
    let observed = quota;
    return unit;
}
