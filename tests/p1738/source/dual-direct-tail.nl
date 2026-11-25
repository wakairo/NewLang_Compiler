struct QueueDescriptor { tag: u8, coalescing: u8 }

fn guarded_bytes(blocks: usize) -> usize {
    if (blocks < usize(1024)) { blocks * usize(64) } else { usize(0) }
}

fn select_mode(bytes: usize) -> u8 {
    if (bytes < usize(4096)) { u8(31) } else { u8(73) }
}

fn exchange_mode(slot: ref<write,u8>, mode: u8) -> u8 {
    replace(slot, mode)
}

fn main() -> unit {
    let active = QueueDescriptor{coalescing: u8(19), tag: u8(83)};
    let standby = QueueDescriptor{coalescing: u8(43), tag: u8(127)};
    let load = usize(3);
    let blocks = if (load < usize(8)) { usize(63) } else { usize(64) };
    let planned = guarded_bytes(blocks);
    let selected = select_mode(planned);
    let initial_active = loan_write(active@coalescing) { |writer| exchange_mode(writer, selected) };
    let reserve_mode = u8(7);
    let initial_standby = loan_write(standby@coalescing) { |writer| exchange_mode(writer, reserve_mode) };
    let active_mode = active@coalescing;
    let active_tag = active@tag;
    let standby_mode = standby@coalescing;
    let standby_tag = standby@tag;
    let previous_active = initial_active;
    let previous_standby = initial_standby;
    return unit;
}
