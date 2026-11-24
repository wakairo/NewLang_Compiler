struct QueueDescriptor { flags: u8, mode: u8 }

fn guarded_bytes(count: usize) -> usize {
    if (count < usize(1024)) {
        count * usize(64)
    } else {
        usize(0)
    }
}

fn queue_mode(bytes: usize) -> u8 {
    if (bytes < usize(4096)) { u8(41) } else { u8(97) }
}

fn main() -> unit {
    let original = QueueDescriptor{mode: u8(11), flags: u8(213)};
    let key = usize(1);
    let count = if (key < usize(2)) { usize(1023) } else { usize(1024) };
    let bytes = guarded_bytes(count);
    let incoming = queue_mode(bytes);
    let old = loan_write(original@mode) { |writer| replace(writer, incoming) };
    let updated = original@mode;
    let sibling = original@flags;
    let old_again = old;
    return unit;
}
