struct ReceiveDescriptor { class_tag: u8, coalescing: u8 }

fn bounded_bytes(blocks: usize) -> usize {
    if (blocks < usize(1024)) { blocks * usize(64) } else { usize(0) }
}

fn negotiated_mode(bytes: usize) -> u8 {
    if (bytes < usize(4096)) { u8(35) } else { u8(93) }
}

fn exchange(slot: ref<write,u8>, mode: u8) -> u8 {
    let old = replace(slot, mode);
    old
}

fn main() -> unit {
    let active = ReceiveDescriptor{coalescing: u8(22), class_tag: u8(119)};
    let standby = ReceiveDescriptor{coalescing: u8(81), class_tag: u8(147)};
    let rank = usize(3);
    let blocks = if (rank < usize(8)) { usize(63) } else { usize(64) };
    let planned = bounded_bytes(blocks);
    let selected = negotiated_mode(planned);
    let old_active = loan_write(active@coalescing) { |field| exchange(field, selected) };
    let applied = standby@coalescing;
    let old_standby = loan_write(standby@coalescing) { |field| exchange(field, applied) };
    let active_mode = active@coalescing;
    let active_tag = active@class_tag;
    let standby_mode = standby@coalescing;
    let standby_tag = standby@class_tag;
    let first_previous = old_active;
    let second_previous = old_standby;
    return unit;
}
