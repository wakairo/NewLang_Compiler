fn measure_grid() -> usize {
    usize(0) * usize(18446744073709551615)
}

fn main() -> unit {
    let count = measure_grid();
    let copy = count;
    copy;
    unit
}
