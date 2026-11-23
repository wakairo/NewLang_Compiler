fn slice(bytes: usize) -> usize {
    bytes / usize(8)
}

fn phase(slices: usize) -> usize {
    slices % usize(7)
}

fn stages(bytes: usize) -> usize {
    let sliced = slice(bytes);
    let phased = phase(sliced);
    phased
}

fn main() -> unit {
    let marker = usize(1);
    let incoming = if (marker < usize(2)) { usize(157) } else { usize(158) };
    let selected = stages(incoming);
    let snapshot = selected;
    snapshot;
    unit
}
