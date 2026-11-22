fn apportion(stock: usize, spare: usize) -> usize {
    let batches = stock / usize(6);
    let remainder = spare % usize(10);
    remainder;
    return batches;
}

fn main() -> unit {
    let stock = usize(66);
    let reserve = usize(15);
    let received = apportion(stock, reserve);
    let pending = received % usize(7);
    unit
}
