fn main() -> unit {
    let octet = u8(173);
    let stored = byte(octet);
    let decoded = u8(stored);
    let quantity = usize(decoded);
    let original = usize(octet);
    let matches = quantity == original;
    unit
}
