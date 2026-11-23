fn measure_grid() -> usize {
    let factor = usize(13);
    usize(29) * factor
}

fn main() -> unit {
    let count = measure_grid();
    let copy = count;
    copy;
    unit
}
