fn main() -> unit {
    let small = parcel(usize(41));
    let large = parcel(usize(57));
    unit
}

fn parcel(amount: usize) -> usize {
    let cutoff = usize(37) + usize(6);
    let chooseleft = amount < cutoff;
    let joined = if (chooseleft) {
        let copied = relay(amount);
        copied % usize(17)
    } else {
        let prepared = usize(101) - usize(8);
        relay(prepared)
    };
    let retained = joined;
    let reduced = retained % usize(19);
    seal(reduced)
}

fn seal(quantity: usize) -> usize {
    let duplicate = quantity;
    duplicate / usize(3)
}

fn relay(quantity: usize) -> usize {
    let duplicate = quantity;
    duplicate
}
