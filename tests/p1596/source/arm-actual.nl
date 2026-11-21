fn bins(count: usize) -> usize {
    count % usize(6)
}

fn main() -> unit {
    let stock = usize(17);
    let chosen = if (stock < usize(40)) {
        bins(stock / usize(5))
    } else {
        bins(stock)
    };
    unit
}
