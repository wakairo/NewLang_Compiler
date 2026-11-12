struct Parcel { active:u8, retained:u8 }
fn relay_current(value:u8) -> u8 {
    let first = value;
    let second = first;
    let saved = second;
    saved
}
fn main() -> unit {
    let original = Parcel { active:u8(91), retained:u8(217) };
    let result = loop(phase=usize(0)) {
        if (phase < usize(3)) {
            let present = original@active;
            let carried = relay_current(present);
            let written = carried;
            let displaced = loan_write(original@active) { |slot|
                replace(slot,written)
            };
            continue(phase+usize(1));
        } else {
            let old = loan_write(original@active) { |slot|
                replace(slot,u8(151))
            };
            break old;
        }
    };
    let current = original@active;
    let sibling = original@retained;
    result;
    current;
    sibling;
    unit
}
