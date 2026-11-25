struct ReceiveQueue { queue_id: u8, mode: u8 }

fn bounded_bytes(blocks: usize) -> usize {
    if (blocks < usize(1024)) { blocks * usize(64) } else { usize(0) }
}

fn select_mode(bytes: usize) -> u8 {
    if (bytes < usize(32768)) { u8(5) } else { u8(17) }
}

fn main() -> unit {
    let descriptor = ReceiveQueue{mode: u8(23), queue_id: u8(191)};
    let rank = usize(11);
    let blocks = if (rank < usize(10)) { usize(511) } else { usize(512) };
    let bytes = bounded_bytes(blocks);
    let selected = select_mode(bytes);
    let old = loan_write(descriptor@mode) { |writer| replace(writer, selected) };
    let stable = descriptor@queue_id;
    let updated = descriptor@mode;
    let old_copy = old;
    return unit;
}
