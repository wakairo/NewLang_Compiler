fn read_window(load: usize) -> u8 {
    if (load < usize(64)) { u8(127) } else { u8(128) }
}

fn window_count(octet: u8) -> usize {
    let count = usize(octet);
    count
}

fn queue_mark(count: usize) -> u8 {
    if (count < usize(128)) { u8(13) } else { u8(29) }
}

fn main() -> unit {
    let load = usize(64);
    let octet = read_window(load);
    let available = window_count(octet);
    let exact = usize(128) == available;
    let below = available < usize(128);
    let queue = queue_mark(available);
    let observed = queue;
    return unit;
}
