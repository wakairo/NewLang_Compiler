fn main() -> unit {
    let size = usize(7);
    let octet = u8(size);
    let old = loan_write(octet) { |w|
        replace(w, u8(9))
    };
    let transferred = usize(octet);
    let expected = usize(9);
    let same = transferred == expected;
    unit
}
