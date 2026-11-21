fn main() -> unit {
    let units = usize(63);
    let selected = if (usize(4) < usize(8)) {
        units
    } else {
        usize(32)
    };
    let copied = selected;
    let grown = copied + usize(1);
    unit
}
