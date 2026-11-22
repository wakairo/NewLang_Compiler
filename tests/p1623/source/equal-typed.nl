fn keep_edge(payload: usize, permit: bool) -> usize {
    let received = payload;
    let choice = if (permit) {
        received
    } else {
        let remaining = received % usize(17);
        remaining
    };
    choice
}

fn main() -> unit {
    let count = usize(1);
    let request = count == usize(1);
    let copied = request;
    let first = keep_edge(count, request);
    let second = keep_edge(count, copied);
    first;
    second;
    unit
}
