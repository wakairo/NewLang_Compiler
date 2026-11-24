struct QueueState { flags: u8, mode: u8 }

fn bounded_bytes(blocks: usize) -> usize {
    if (blocks < usize(1024)) { blocks * usize(64) } else { usize(0) }
}

fn select_mode(bytes: usize) -> u8 {
    if (bytes < usize(4096)) { u8(53) } else { u8(109) }
}

fn install_mode(destination: ref<write,u8>, chosen: u8) -> u8 {
    let previous = replace(destination, chosen);
    previous
}

fn main() -> unit {
    let original = QueueState{mode: u8(17), flags: u8(211)};
    let key = usize(4);
    let blocks = if (key < usize(5)) { usize(1023) } else { usize(1024) };
    let bytes = bounded_bytes(blocks);
    let selected = select_mode(bytes);
    let old = loan_write(original@mode) { |writer| install_mode(writer, selected) };
    let updated = original@mode;
    let sibling = original@flags;
    let old_again = old;
    return unit;
}
