struct RxDescriptor { queue_tag: u8, copy_limit: u8 }

fn bounded_bytes(blocks: usize) -> usize {
    if (blocks < usize(256)) { blocks * usize(128) } else { usize(0) }
}

fn pick_mode(bytes: usize) -> u8 {
    if (bytes < usize(16384)) { u8(6) } else { u8(31) }
}

fn set_mode(slot: ref<write,u8>, selected: u8) -> u8 {
    replace(slot, u8(31))
}

fn main() -> unit {
    let descriptor = RxDescriptor{copy_limit: u8(11), queue_tag: u8(91)};
    let rank = usize(5);
    let blocks = if (rank < usize(7)) { usize(255) } else { usize(256) };
    let bytes = bounded_bytes(blocks);
    let selected = pick_mode(bytes);
    let old = loan_write(descriptor@copy_limit) { |writer| set_mode(writer, selected) };
    let updated = descriptor@copy_limit;
    let unchanged = descriptor@queue_tag;
    let old_copy = old;
    return unit;
}
