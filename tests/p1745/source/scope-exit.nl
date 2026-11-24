struct ReceiveQueue { queue_id: u8, coalescing: u8 }

fn main() -> unit {
    let queue = ReceiveQueue{queue_id: u8(37), coalescing: u8(9)};
    let retained = loan_write(queue@coalescing) { |writer| writer };
    let old = replace(retained, u8(1));
    return unit;
}
