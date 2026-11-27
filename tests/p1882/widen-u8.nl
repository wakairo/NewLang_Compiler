fn main() -> unit {
    let octet = u8(42);
    let count = usize(octet);
    let expected = usize(42);
    let same = count == expected;
    unit
}
