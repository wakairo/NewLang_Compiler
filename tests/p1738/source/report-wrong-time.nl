struct ReceiveQueue { tag: u8, coalescing: u8 }

fn bounded_bytes(blocks: usize) -> usize {
    if (blocks < usize(1024)) { blocks * usize(64) } else { usize(0) }
}

fn choose_mode(bytes: usize) -> u8 {
    if (bytes < usize(4096)) { u8(13) } else { u8(31) }
}

fn install_mode(slot: ref<write,u8>, mode: u8) -> u8 {
    let displaced = replace(slot, mode);
    displaced
}

fn main() -> unit {
    let queue = ReceiveQueue{coalescing: u8(19), tag: u8(19)};
    let rank = usize(3);
    let blocks = if (rank < usize(7)) { usize(63) } else { usize(64) };
    let bytes = bounded_bytes(blocks);
    let selected = choose_mode(bytes);
    let first = loan_write(queue@coalescing) { |slot| install_mode(slot, selected) };
    let intermediate = queue@coalescing;
    let midpoint_tag = queue@tag;
    let recovery = u8(19);
    let second = loan_write(queue@coalescing) { |slot| install_mode(slot, recovery) };
    let installed = queue@coalescing;
    let tag = queue@tag;
    let first_copy = first;
    let second_copy = second;
    let reported_mode = installed;
    let reported_tag = midpoint_tag;
    return unit;
}
