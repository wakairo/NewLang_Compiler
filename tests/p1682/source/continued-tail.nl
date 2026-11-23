fn measure_grid() -> usize {
    let product = usize(23) * usize(17);
    product
}

fn main() -> unit {
    let count = measure_grid();
    let copy = count;
    copy;
    unit
}
