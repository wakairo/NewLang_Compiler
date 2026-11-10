struct Beacon { guard:u8, value:u8 }
fn choose_byte(phase:usize) -> u8 {
    if (phase < usize(1)) { u8(85) } else { u8(173) }
}
fn main() -> unit {
    let beacon = Beacon { guard:u8(59), value:u8(23) };
    let old_result = loop(phase=usize(0)) {
        if (phase < usize(2)) {
            let next = choose_byte(phase);
            let dropped = loan_write(beacon@value) { |slot|
                replace(slot,next)
            };
            continue(phase+usize(1));
        } else {
            let old = loan_write(beacon@value) { |slot|
                replace(slot,u8(211))
            };
            break old;
        }
    };
    let live = beacon@value;
    let unchanged = beacon@guard;
    let copied_old = old_result;
    live;
    unchanged;
    copied_old;
    unit
}
