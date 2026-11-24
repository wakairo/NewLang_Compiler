struct QueueDescriptor { mode: u8, flags: u8 }

fn select_mode(bytes: usize) -> u8 {
    if (bytes < usize(4096)) { u8(41) } else { u8(97) }
}

fn install_mode(target: ref<write,u8>, incoming: u8) -> u8 {
    let displaced = replace(target, incoming);
    u8(11)
}

fn main() -> unit {
    let original = QueueDescriptor{mode: u8(11), flags: u8(213)};
    let bytes = usize(65472);
    let incoming = select_mode(bytes);
    let old = loan_write(original@mode) { |writer| install_mode(writer, incoming) };
    let updated = original@mode;
    let sibling = original@flags;
    let old_again = old;
    return unit;
}
