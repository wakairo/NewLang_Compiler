fn measure_grid(stock: usize) -> usize {
    let portion = stock / usize(6);
    portion
}

fn main() -> unit {
    let stock = usize(83);
    let count = measure_grid(stock);
    unit
}
