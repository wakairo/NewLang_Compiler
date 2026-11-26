struct ReceiveQueue { tag: u8, batching: u8 }

fn guarded_bytes(blocks: usize) -> usize {
    if (blocks < usize(1024)) { blocks * usize(64) } else { usize(0) }
}

fn trial_mode(bytes: usize) -> u8 {
    if (bytes < usize(4096)) { u8(13) } else { u8(31) }
}

fn write_mode(slot: ref<write,u8>, mode: u8) -> u8 {
    let old = replace(slot, mode);
    old
}

fn main() -> unit {
    let queue = ReceiveQueue{batching: u8(27), tag: u8(27)};
    let rank = usize(3);
    let blocks = if (rank < usize(7)) { usize(63) } else { usize(64) };
    let bytes = guarded_bytes(blocks);
    let trial = trial_mode(bytes);
    let first = loan_write(queue@batching) { |slot| write_mode(slot, trial) };
    let during = queue@batching;
    let during_tag = queue@tag;
    let restore = first;
    let second = loan_write(queue@batching) { |slot| write_mode(slot, restore) };
    let restored = queue@batching;
    let unchanged = queue@tag;
    let first_old = first;
    let second_old = second;
    let report_mode = during;
    let report_tag = during_tag;
    return unit;
}
