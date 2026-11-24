struct QueueDescriptor { mode: u8, flags: u8 }

fn queue_mode(bytes: usize) -> u8 {
    if (bytes < usize(4096)) { u8(41) } else { u8(97) }
}

fn main() -> unit {
    let original = QueueDescriptor{mode: u8(11), flags: u8(213)};
    let bytes = usize(65472);
    let incoming = queue_mode(bytes);
    let old = loan_write(original@mode) { |writer| replace(writer, u8(97)) };
    let updated = original@mode;
    let sibling = original@flags;
    let old_again = old;
    return unit;
}
