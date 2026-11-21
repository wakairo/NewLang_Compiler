fn bins(count: usize) -> usize {
    count % usize(6)
}

fn demand(order: usize) -> usize {
    let stock = order;
    let first = bins(stock / usize(5));
    let ahead = if (stock < usize(49)) { first } else { stock };
    return bins(ahead % usize(11));
}

fn main() -> unit {
    let small = demand(usize(47));
    let large = demand(usize(82));
    return unit;
}
