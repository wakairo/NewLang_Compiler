fn deliver(parcel: usize) -> usize {
    parcel
}

fn relay(gross: usize) -> usize {
    let amount = gross;
    let discounted = amount / usize(2);
    let ready = if (amount < usize(50)) { amount } else { discounted };
    let settled = deliver(deliver(ready));
    settled % usize(13)
}

fn main() -> unit {
    let early = relay(usize(34));
    let late = relay(usize(76));
    unit
}
