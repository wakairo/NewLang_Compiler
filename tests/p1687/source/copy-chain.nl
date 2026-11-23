fn panel_count() -> usize {
    let pitch = usize(31);
    let rows = usize(19);
    let area = pitch * rows;
    let saved = area;
    let copy = saved;
    copy
}

fn main() -> unit {
    let count = panel_count();
    panel_count();
    let second = panel_count();
    let copy = second;
    copy;
    unit
}
