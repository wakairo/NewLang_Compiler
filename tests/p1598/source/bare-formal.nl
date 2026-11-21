fn tag(ticket: usize) -> usize {
    ticket
}

fn main() -> unit {
    let budget = usize(143);
    let saved = tag(budget);
    let flag = saved % usize(12);
    unit
}
