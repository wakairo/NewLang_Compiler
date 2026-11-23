fn measure_grid() -> usize {
    let pitch = usize(23);
    let rows = usize(17);
    let width = pitch;
    width * rows
}

fn relay_grid(value: usize) -> usize {
    value
}

fn main() -> unit {
    let count = relay_grid(measure_grid());
    unit
}
