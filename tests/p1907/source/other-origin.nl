fn read_window(load: usize) -> u8 {
    if (load < usize(64)) { u8(127) } else { u8(128) }
}

fn main() -> unit {
    let load = usize(64);
    let octet = read_window(load);
    let available = usize(octet);
    let exact = available == usize(128);
    return unit;
}
