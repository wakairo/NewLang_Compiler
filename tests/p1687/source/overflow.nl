fn panel_count() -> usize {
    let area = usize(6148914691236517206) * usize(3);
    area
}

fn main() -> unit {
    let count = panel_count();
    let copy = count;
    copy;
    unit
}
