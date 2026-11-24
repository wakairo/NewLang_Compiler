struct QueueDescriptor { flags: u8, mode: u8 }

fn main() -> unit {
    let original = QueueDescriptor{mode: u8(11), flags: u8(213)};
    let bytes = usize(0);
    let below = bytes < usize(4096);
    let incoming = if (below) { u8(41) } else { u8(97) };
    let old = loan_write(original@mode) { |writer| replace(writer, incoming) };
    let updated = original@mode;
    let sibling = original@flags;
    let old_again = old;
    return unit;
}
