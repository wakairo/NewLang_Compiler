fn main() -> unit {
    let pallets = usize(78);
    let selected = if (pallets < usize(70)) {
        pallets / usize(0)
    } else {
        pallets % usize(5)
    };
    unit
}
