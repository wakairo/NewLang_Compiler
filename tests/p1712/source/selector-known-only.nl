fn queue_mode(bytes: usize) -> u8 {
    if (bytes < usize(4096)) { u8(41) } else { u8(97) }
}

fn main() -> unit {
    let bytes = usize(65472);
    let incoming = queue_mode(bytes);
    let observed = incoming;
    return unit;
}
