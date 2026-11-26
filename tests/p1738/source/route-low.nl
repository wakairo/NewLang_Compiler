struct QueueStatus { queue_tag: u8, batching: u8 }

fn bounded_bytes(blocks: usize) -> usize {
    if (blocks < usize(1024)) { blocks * usize(64) } else { usize(0) }
}

fn trial_mode(bytes: usize) -> u8 {
    if (bytes < usize(4096)) { u8(13) } else { u8(31) }
}

fn exchange(slot: ref<write,u8>, next: u8) -> u8 {
    let old = replace(slot, next);
    old
}

fn main() -> unit {
    let queue = QueueStatus{batching: u8(27), queue_tag: u8(27)};
    let rank = usize(3);
    let blocks = if (rank < usize(7)) { usize(63) } else { usize(64) };
    let bytes = bounded_bytes(blocks);
    let temporary = trial_mode(bytes);
    let first = loan_write(queue@batching) { |slot| exchange(slot, temporary) };
    let mode_during = queue@batching;
    let tag_during = queue@queue_tag;
    let restore = u8(27);
    let second = loan_write(queue@batching) { |slot| exchange(slot, restore) };
    let restored = queue@batching;
    let tag_now = queue@queue_tag;
    let first_old = first;
    let second_old = second;
    let report_mode = mode_during;
    let report_tag = tag_during;
    let large_trial = usize(4064) < bytes;
    return unit;
}
