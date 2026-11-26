struct ReceiveQueue { tag: u8, batching: u8 }

fn bounded_bytes(blocks: usize) -> usize {
    if (blocks < usize(1024)) { blocks * usize(64) } else { usize(0) }
}

fn trial_mode(bytes: usize) -> u8 {
    if (bytes < usize(4096)) { u8(13) } else { u8(31) }
}

fn exchange_mode(slot: ref<write,u8>, mode: u8) -> u8 {
    let old = replace(slot, mode);
    old
}

fn main() -> unit {
    let queue = ReceiveQueue{batching: u8(61), tag: u8(73)};
    let rank = usize(3);
    let blocks = if (rank < usize(7)) { usize(63) } else { usize(64) };
    let bytes = bounded_bytes(blocks);
    let temporary = trial_mode(bytes);
    let first = loan_write(queue@batching) { |slot| exchange_mode(slot, temporary) };
    let mode_during = queue@batching;
    let tag_during = queue@tag;
    let recovery = first;
    let second = loan_write(queue@batching) { |slot| exchange_mode(slot, recovery) };
    let restored = queue@batching;
    let unchanged = queue@tag;
    let first_old = first;
    let second_old = second;
    let report_mode = mode_during;
    let report_tag = tag_during;
    let large_trial = usize(4064) < bytes;
    let payload = if (large_trial) { report_tag } else { report_mode };
    return unit;
}
