struct Reading { pinned:u8, active:u8 }
fn copy_current(value:u8) -> u8 {
    let saved = value;
    saved
}
fn main() -> unit {
    let original = Reading { pinned:u8(181), active:u8(37) };
    let result = loop(phase=usize(0)) {
        if (phase < usize(2)) {
            let present = original@active;
            let carried = copy_current(present);
            let displaced = loan_write(original@active) { |slot|
                replace(slot,carried)
            };
            continue(phase+usize(1));
        } else {
            let old = loan_write(original@active) { |slot|
                replace(slot,u8(203))
            };
            break old;
        }
    };
    let current = original@active;
    let sibling = original@pinned;
    result;
    current;
    sibling;
    unit
}
