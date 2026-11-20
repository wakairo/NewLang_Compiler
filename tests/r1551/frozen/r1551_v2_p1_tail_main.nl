fn cedar_tail(stock: usize) -> usize {
    let quotient = stock / usize(9);
    let held = quotient;
    let outgoing = held;
    outgoing
}

fn main() -> unit {
    let first = cedar_tail(usize(2017));
    let preserved = first;
    let second = cedar_tail(preserved);
    let raised = first + usize(17);
    let adjusted = raised - usize(9);
    let third = cedar_tail(adjusted);
    unit
}
