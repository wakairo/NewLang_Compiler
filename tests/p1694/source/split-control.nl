fn payload_size() -> usize {
    let columns = usize(37);
    let payload = columns * usize(29);
    payload
}

fn slots(input: usize) -> usize {
    input / usize(31)
}

fn main() -> unit {
    let payload = payload_size();
    let result = slots(payload);
    result;
    unit
}
