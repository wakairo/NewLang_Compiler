fn trim(value: usize) -> usize {
    value % usize(13)
}

fn settle(inflow: usize) -> usize {
    let available = inflow;
    let result = if (available < usize(100)) {
        let quotient = available / usize(3);
        trim(quotient)
    } else {
        let carried = available;
        trim(carried)
    };
    result
}

fn main() -> unit {
    let lower = settle(usize(86));
    let higher = settle(usize(115));
    unit
}
