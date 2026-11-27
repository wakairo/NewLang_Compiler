fn read_window(load: usize) -> u8 {
    if (load < usize(64)) { u8(127) } else { u8(128) }
}

fn window_count(octet: u8) -> usize {
    let count = usize(octet);
    count
}

fn main() -> unit {
    let load = usize(64);
    let octet = read_window(load);
    let available = window_count(octet);
    let bad = available == octet;
    return unit;
}
