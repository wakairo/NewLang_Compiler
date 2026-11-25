struct PortStatus { moderation: u8, affinity: u8 }

fn bounded_bytes(blocks: usize) -> usize {
    if (blocks < usize(512)) { blocks * usize(128) } else { usize(0) }
}

fn select_mode(bytes: usize) -> u8 {
    if (bytes < usize(32768)) { u8(2) } else { u8(12) }
}

fn write_mode(slot: ref<write,u8>, selected: u8) -> u8 {
    let displaced = replace(slot, selected);
    displaced
}

fn main() -> unit {
    let descriptor = PortStatus{moderation: u8(63), affinity: u8(207)};
    let rank = usize(11);
    let blocks = if (rank < usize(10)) { usize(511) } else { usize(512) };
    let bytes = bounded_bytes(blocks);
    let selected = select_mode(bytes);
    let old = loan_write(descriptor@moderation) { |writer| write_mode(writer, selected) };
    let stable = descriptor@affinity;
    let updated = descriptor@moderation;
    let old_copy = old;
    return unit;
}
