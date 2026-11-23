fn bytes_for(count: usize) -> usize {
    if (count < usize(2)) {
        count * usize(18446744073709551615)
    } else {
        usize(0)
    }
}

fn queue_for(count: usize) -> usize {
    let bytes = bytes_for(count);
    if (bytes < usize(1)) { usize(7) } else { usize(9) }
}

fn main() -> unit {
    let marker = usize(1);
    let incoming = if (marker < usize(2)) { usize(1) } else { usize(2) };
    let selected = queue_for(incoming);
    let snapshot = selected;
    snapshot;
    unit
}
