fn trays(load: usize) -> usize {
    let held = load;
    held % usize(8)
}

fn main() -> unit {
    let parcels = usize(95);
    let stock = parcels;
    let quotient = trays(stock / usize(6));
    let remainder = trays(stock % usize(16));
    let added = trays(usize(11) + usize(9));
    let subtracted = trays(stock - usize(72));
    unit
}
