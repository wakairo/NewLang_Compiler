fn measure_grid() -> usize {
    let octet = u8(23);
    octet * usize(17)
}

fn main() -> unit {
    let count = measure_grid();
    let copy = count;
    copy;
    unit
}
