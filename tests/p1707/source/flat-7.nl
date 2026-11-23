fn bytes_for(count: usize) -> usize {
    if (count < usize(8)) {
        count * usize(512)
    } else {
        usize(0)
    }
}

fn main() -> unit {
    let marker = usize(1);
    let incoming = if (marker < usize(2)) { usize(7) } else { usize(8) };
    let bytes = bytes_for(incoming);
    let selected = if (bytes < usize(3072)) { usize(7) } else { usize(9) };
    let snapshot = selected;
    snapshot;
    unit
}
