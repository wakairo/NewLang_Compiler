fn main() -> unit {
    let intake = usize(519);
    let spare = usize(173);
    let chosen = tickets(intake);
    let copied = chosen;
    let batches = copied / usize(4);
    let residue = batches % usize(7);
    let finish = leftover(copied);
    let reserve = scraps(spare);
    let forwarded = reserve;
    let reserve_batches = forwarded / usize(4);
    let reserve_finish = leftover(forwarded);
    unit
}

fn tickets(amount: usize) -> usize {
    let ready = amount / usize(7);
    ready
}

fn scraps(amount: usize) -> usize {
    let ready = amount % usize(37);
    ready
}

fn leftover(amount: usize) -> usize {
    let ready = amount % usize(11);
    ready
}
