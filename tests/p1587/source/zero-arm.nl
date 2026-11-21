fn main() -> unit {
    let units = usize(57);
    let selected = if (usize(3) < usize(4)) {
        units
    } else {
        let impossible = units % usize(0);
        impossible
    };
    unit
}
