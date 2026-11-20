fn packets_for(bytes: usize) -> usize {
    let packets = bytes / usize(6);
    let snapshot = packets;
    snapshot
}

fn main() -> unit {
    let available = usize(47);
    let batches = packets_for(available);
    let saved_batches = batches;
    let with_header = saved_batches + usize(3);
    let remaining = with_header - usize(2);
    unit
}
