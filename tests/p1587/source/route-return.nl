fn main() -> unit {
    let small = route(usize(49));
    let large = route(usize(221));
    let remainder = large % usize(4);
    return unit;
}

fn route(amount: usize) -> usize {
    let parcel = amount;
    let near = parcel < usize(128);
    let joined = if (near) {
        tickets(parcel)
    } else {
        let excess = scraps(parcel);
        excess
    };
    let sent = joined;
    let boxes = sent / usize(2);
    return wrap(boxes);
}

fn tickets(amount: usize) -> usize {
    let ready = amount / usize(7);
    ready
}

fn scraps(amount: usize) -> usize {
    let ready = amount % usize(37);
    ready
}

fn wrap(amount: usize) -> usize {
    let ready = amount % usize(13);
    ready
}
