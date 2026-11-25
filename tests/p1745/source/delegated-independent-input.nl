struct ReceiveQueue { queue_id: u8, coalescing: u8 }

fn receive_budget(blocks: usize) -> usize {
    if (blocks < usize(1024)) {
        blocks * usize(64)
    } else {
        usize(0)
    }
}

fn coalescing_band(input_bytes: usize) -> u8 {
    if (input_bytes < usize(4096)) {
        u8(1)
    } else {
        u8(4)
    }
}

fn coalescing_policy(bytes: usize) -> u8 { coalescing_band(usize(4096)) }

fn configure_coalescing(destination: ref<write,u8>, budget: usize) -> u8 {
    let mode = coalescing_policy(budget);
    let displaced = replace(destination, mode);
    displaced
}

fn main() -> unit {
    let queue = ReceiveQueue{queue_id: u8(37), coalescing: u8(9)};
    let rank = usize(11);
    let blocks = if (rank < usize(7)) { usize(63) } else { usize(64) };
    let bytes = receive_budget(blocks);
    let old = loan_write(queue@coalescing) { |writer| configure_coalescing(writer, bytes) };
    let current = queue@coalescing;
    let lane = queue@queue_id;
    let displaced_again = old;
    return unit;
}
