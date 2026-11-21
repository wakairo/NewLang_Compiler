fn main() -> unit {
    let low = route(route(usize(17)));
    let high = route(route(usize(65)));
    let boundary = route(route(usize(9)));
    let zero = route(route(usize(0)));
    unit
}

fn route(amount: usize) -> usize {
    if (amount < usize(9)) {
        amount % usize(5)
    } else {
        amount / usize(6)
    }
}
