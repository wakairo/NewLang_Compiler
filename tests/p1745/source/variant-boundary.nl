struct PortStatus { moderation: u8, affinity: u8 }

fn pool_budget(batches: usize) -> usize {
    if (batches < usize(2048)) { batches * usize(16) } else { usize(0) }
}

fn tune_port(slot: ref<write,u8>, bytes: usize) -> u8 {
    let setting = if (bytes < usize(2048)) { u8(2) } else { u8(7) };
    let previous = replace(slot, setting);
    previous
}

fn main() -> unit {
    let port = PortStatus{moderation: u8(17), affinity: u8(51)};
    let sample = usize(7);
    let batches = if (sample < usize(5)) { usize(127) } else { usize(128) };
    let allocated = pool_budget(batches);
    let previous = loan_write(port@moderation) { |output| tune_port(output, allocated) };
    let active = port@moderation;
    let sticky = port@affinity;
    let previous_copy = previous;
    return unit;
}
