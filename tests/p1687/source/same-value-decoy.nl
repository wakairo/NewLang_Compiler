fn panel_count() -> usize {
    let pitch = usize(31);
    let rows = usize(19);
    let area = pitch * rows;
    let decoy = usize(589);
    decoy
}

fn main() -> unit {
    let count = panel_count();
    let copy = count;
    copy;
    unit
}
