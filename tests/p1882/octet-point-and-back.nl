fn main() -> unit {
    let octet = u8(255);
    let point = uintptr(octet);
    let again = u8(point);
    let size = usize(point);
    let expected = usize(255);
    let same = size == expected;
    unit
}
