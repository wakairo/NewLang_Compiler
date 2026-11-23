fn measure_grid() -> usize {
    let pitch = usize(23);
    let rows = usize(17);
    let width = pitch;
    width * rows
}

fn main() -> unit {
    let first = measure_grid();
    measure_grid();
    let second = measure_grid();
    let copy = second;
    copy;
    unit
}
