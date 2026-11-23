fn measure_grid() -> usize {
    let pitch = usize(23);
    let rows = usize(17);
    let width = pitch;
    width * rows
}

fn relay_grid() -> usize {
    measure_grid()
}

fn main() -> unit {
    let count = relay_grid();
    unit
}
