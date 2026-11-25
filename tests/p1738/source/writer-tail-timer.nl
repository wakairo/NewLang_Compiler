struct TimerDescriptor { pacing: u8, affinity: u8 }

fn bounded_bytes(blocks: usize) -> usize {
    if (blocks < usize(512)) { blocks * usize(64) } else { usize(0) }
}

fn pick_mode(bytes: usize) -> u8 {
    if (bytes < usize(8192)) { u8(7) } else { u8(63) }
}

fn set_mode(slot: ref<write,u8>, selected: u8) -> u8 {
    replace(slot, selected)
}

fn main() -> unit {
    let descriptor = TimerDescriptor{pacing: u8(73), affinity: u8(229)};
    let rank = usize(9);
    let blocks = if (rank < usize(7)) { usize(511) } else { usize(512) };
    let bytes = bounded_bytes(blocks);
    let selected = pick_mode(bytes);
    let old = loan_write(descriptor@pacing) { |writer| set_mode(writer, selected) };
    let updated = descriptor@pacing;
    let unchanged = descriptor@affinity;
    let old_copy = old;
    return unit;
}
