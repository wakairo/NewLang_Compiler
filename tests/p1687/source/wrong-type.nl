fn panel_count() -> usize {
    let octet = u8(31);
    let area = octet * usize(19);
    area
}

fn main() -> unit {
    let count = panel_count();
    let copy = count;
    copy;
    unit
}
