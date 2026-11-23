fn panel_count() -> usize {
    let span = usize(6148914691236517205);
    let area = usize(3) * span;
    area
}

fn main() -> unit {
    let count = panel_count();
    let copy = count;
    copy;
    unit
}
