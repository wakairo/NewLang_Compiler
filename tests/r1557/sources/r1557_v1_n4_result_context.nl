fn narrowed_result() -> u8 {
    let shipment = usize(79);
    shipment % usize(12)
}

fn main() -> unit {
    let rejected = narrowed_result();
    unit
}
