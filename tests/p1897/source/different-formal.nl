fn widen_quota(octet: u8) -> usize {
    let other = u8(1);
    usize(other)
}

fn main() -> unit {
    let octet = u8(255);
    let quota = widen_quota(octet);
    let observed = quota;
    return unit;
}
