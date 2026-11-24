struct QueueDescriptor { mode: u8, flags: u8 }

fn main() -> unit {
    let original = QueueDescriptor{mode: u8(11), flags: u8(213)};
    let outside = loan_write(original@mode) { |writer| writer };
    let old = replace(outside, u8(97));
    let updated = original@mode;
    let sibling = original@flags;
    let old_again = old;
    return unit;
}
