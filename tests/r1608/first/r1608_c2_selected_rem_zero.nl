fn main() -> unit {
    let seed = usize(53);
    let copied = seed;
    let selected = if (usize(1) == usize(2)) {
        copied / usize(7)
    } else {
        copied % usize(0)
    };
    selected;
    seed;
    unit
}
