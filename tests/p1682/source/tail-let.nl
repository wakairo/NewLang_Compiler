fn measure_grid() -> usize {
    let pitch = usize(23);
    let rows = usize(17);
    let width = pitch;
    width * rows
}

fn main() -> unit {
    let count = measure_grid();
    let copy = count;
    copy;
    unit
}
