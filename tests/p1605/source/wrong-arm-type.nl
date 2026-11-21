fn main() -> unit {
    let pallets = usize(78);
    let pieces = u8(17);
    let selected = if (pallets < usize(80)) {
        pieces % usize(5)
    } else {
        pallets
    };
    unit
}
