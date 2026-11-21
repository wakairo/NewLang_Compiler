fn bins(count: usize) -> usize {
    count % usize(6)
}

fn main() -> unit {
    let stock = usize(17);
    let value = bins(bins(stock) % usize(3));
    unit
}
