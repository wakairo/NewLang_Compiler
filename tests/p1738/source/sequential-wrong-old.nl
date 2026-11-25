struct ReceiveQueue { tag: u8, coalescing: u8 }

fn bounded_bytes(blocks: usize) -> usize {
    if (blocks < usize(1024)) { blocks * usize(64) } else { usize(0) }
}

fn choose_mode(bytes: usize) -> u8 {
    if (bytes < usize(4096)) { u8(13) } else { u8(31) }
}

fn install_mode(slot: ref<write,u8>, selected: u8) -> u8 {
    let displaced = replace(slot, selected);
    displaced
}

fn main() -> unit {
    let queue = ReceiveQueue{coalescing: u8(9), tag: u8(97)};
    let rank = usize(11);
    let blocks = if (rank < usize(7)) { usize(63) } else { usize(64) };
    let bytes = bounded_bytes(blocks);
    let selected = choose_mode(bytes);
    let first = loan_write(queue@coalescing) { |writer| install_mode(writer, selected) };
    let intermediate = queue@coalescing;
    let recovery = u8(5);
    let second = loan_write(queue@coalescing) { |writer| install_mode(writer, recovery) };
    let updated = queue@coalescing;
    let unchanged = queue@tag;
    let first_copy = first;
    let second_copy = first;
    return unit;
}
