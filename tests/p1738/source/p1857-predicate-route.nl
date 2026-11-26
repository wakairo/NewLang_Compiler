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

fn report_open(limit: usize, byte_count: usize) -> bool {
    limit < byte_count
}

fn main() -> unit {
    let route_limit = usize(4000);
    let delivery_limit = usize(4064);
    let queue = ReceiveQueue{batching: u8(67), tag: u8(101)};
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
    let large_trial = report_open(route_limit, bytes);
    let payload = if (large_trial) { report_tag } else { report_mode };
    let report_input = payload;
    let report_default = u8(255);
    let report_ready = delivery_limit < bytes;
    let report_byte = if (report_ready) { report_input } else { report_default };
    let report_used_history = report_ready;
    return unit;
}
