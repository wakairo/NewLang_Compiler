fn widen_quota(octet: u8) -> usize {
    let quota = usize(octet);
    usize(0)
}

fn main() -> unit {
    let octet = u8(255);
    let quota = widen_quota(octet);
    let observed = quota;
    return unit;
}
