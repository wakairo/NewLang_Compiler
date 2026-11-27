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
    let current = available;
    let limit = usize(128);
    let exact = current == limit;
    let below = current < usize(128);
    let above = usize(127) < current;
    let remembered = exact;
    let current_again = current;
    return unit;
}
