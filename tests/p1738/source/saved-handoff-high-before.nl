struct ReceiveQueue { tag: u8, batching: u8 }

fn bounded_bytes(blocks: usize) -> usize {
    if (blocks < usize(1024)) { blocks * usize(64) } else { usize(0) }
}

fn trial_mode(bytes: usize) -> u8 {
    if (bytes < usize(4096)) { u8(17) } else { u8(39) }
}

fn exchange_mode(slot: ref<write,u8>, mode: u8) -> u8 {
    let old = replace(slot, mode);
    old
}

fn main() -> unit {
    let queue = ReceiveQueue{tag: u8(41), batching: u8(151)};
    let rank = usize(9);
    let blocks = if (rank < usize(7)) { usize(63) } else { usize(64) };
    let bytes = bounded_bytes(blocks);
    let temporary = trial_mode(bytes);
    let first = loan_write(queue@tag) { |slot| exchange_mode(slot, temporary) };
    let mode_during = queue@tag;
    let tag_during = queue@batching;
    let recovery = first;
    let second = loan_write(queue@tag) { |slot| exchange_mode(slot, recovery) };
    let restored = queue@tag;
    let unchanged = queue@batching;
    let first_old = first;
    let second_old = second;
    let report_mode = mode_during;
    let report_tag = tag_during;
    let large_trial = usize(4064) < bytes;
    let payload = if (large_trial) { report_tag } else { report_mode };
    let report_input = payload;
    return unit;
}
