fn panel_count() -> usize {
    let bound = usize(90);
    let flag = usize(63) < bound;
    let span = if (flag) { usize(23) } else { usize(29) };
    let area = span * usize(11);
    area
}

fn main() -> unit {
    let count = panel_count();
    let copy = count;
    copy;
    unit
}
