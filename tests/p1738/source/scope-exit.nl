struct QueueState { mode: u8, flags: u8 }

fn main() -> unit {
    let original = QueueState{mode: u8(17), flags: u8(211)};
    let outside = loan_write(original@mode) { |writer| writer };
    let old = replace(outside, u8(53));
    let updated = original@mode;
    let sibling = original@flags;
    let old_again = old;
    return unit;
}
