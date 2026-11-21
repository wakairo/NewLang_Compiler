fn retail(count: usize) -> usize {
    count / usize(3)
}

fn stock(count: usize) -> usize {
    count % usize(14)
}

fn main() -> unit {
    let ballots = usize(85);
    let value = stock(retail(ballots % usize(23)));
    unit
}
