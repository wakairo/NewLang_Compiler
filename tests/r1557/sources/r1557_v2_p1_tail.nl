fn parcel_tail(amount: usize) -> usize {
    let residual = amount % usize(17);
    let forwarded = residual;
    forwarded
}

fn main() -> unit {
    let first_result = parcel_tail(usize(143));
    let second_result = parcel_tail(usize(153));
    let first_copy = first_result;
    let second_copy = second_result;
    unit
}
