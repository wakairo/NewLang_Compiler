fn discard(goods: usize) -> usize {
    let actual = goods / usize(7);
    let substituted = usize(4);
    let cells = substituted % usize(5);
    cells
}

fn main() -> unit {
    let result = discard(usize(28));
    unit
}
