fn bins(count: usize) -> usize {
    count % usize(6)
}

fn demand(order: usize) -> usize {
    bins(order + usize(1))
}

fn main() -> unit {
    let small = demand(usize(17));
    unit
}
