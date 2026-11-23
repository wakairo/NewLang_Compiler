fn bytes_for(count: usize) -> usize {
    if (count < usize(8)) {
        count * usize(512)
    } else {
        usize(0)
    }
}

fn queue_for(count: usize) -> usize {
    let bytes = bytes_for(count);
    if (bytes < usize(3072)) { usize(7) } else { usize(9) }
}

fn extra(count: usize) -> usize {
    queue_for(count)
}

fn main() -> unit {
    let marker = usize(1);
    let incoming = if (marker < usize(2)) { usize(7) } else { usize(8) };
    let selected = extra(incoming);
    let snapshot = selected;
    snapshot;
    unit
}
