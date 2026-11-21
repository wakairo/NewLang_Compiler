fn bins(count: usize) -> usize {
    count % usize(6)
}

fn main() -> unit {
    let stock = usize(17);
    let value = bins(stock % usize(0));
    unit
}
