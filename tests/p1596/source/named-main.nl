fn trays(load: usize) -> usize {
    let held = load;
    held % usize(8)
}

fn main() -> unit {
    let parcels = usize(95);
    let stock = parcels;
    let divided = stock / usize(6);
    let quotient = trays(divided);
    let residue = stock % usize(16);
    let remainder = trays(residue);
    let sum = usize(11) + usize(9);
    let added = trays(sum);
    let difference = stock - usize(72);
    let subtracted = trays(difference);
    unit
}
