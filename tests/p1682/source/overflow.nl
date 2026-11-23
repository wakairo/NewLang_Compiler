fn measure_grid() -> usize {
    let span = usize(6148914691236517206);
    span * usize(3)
}

fn main() -> unit {
    let count = measure_grid();
    let copy = count;
    copy;
    unit
}
