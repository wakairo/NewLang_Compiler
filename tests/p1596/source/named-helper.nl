fn bins(count: usize) -> usize {
    count % usize(6)
}

fn demand(order: usize) -> usize {
    let stock = order;
    let divided = stock / usize(5);
    let first = bins(divided);
    let ahead = if (stock < usize(49)) { first } else { stock };
    let residue = ahead % usize(11);
    return bins(residue);
}

fn main() -> unit {
    let small = demand(usize(47));
    let large = demand(usize(82));
    return unit;
}
