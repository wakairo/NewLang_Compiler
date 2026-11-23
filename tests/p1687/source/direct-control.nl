fn panel_count() -> usize {
    let pitch = usize(31);
    let rows = usize(19);
    pitch * rows
}

fn main() -> unit {
    let count = panel_count();
    let copy = count;
    copy;
    unit
}
