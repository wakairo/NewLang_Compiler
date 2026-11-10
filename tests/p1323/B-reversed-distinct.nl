struct Relay { left:u8, right:u8 }
fn colour(phase:usize) -> u8 {
    if (phase < usize(1)) { u8(73) } else { u8(163) }
}
fn main() -> unit {
    let relay = Relay { left:u8(31), right:u8(211) };
    let answer = loop(phase=usize(0)) {
        if (phase < usize(2)) {
            let next = colour(phase);
            let old_right = loan_write(relay@right) { |slot|
                replace(slot,next)
            };
            let old_left = loan_write(relay@left) { |slot|
                replace(slot,u8(109))
            };
            continue(phase+usize(1));
        } else {
            let old = loan_write(relay@right) { |slot|
                replace(slot,u8(241))
            };
            break old;
        }
    };
    let left_after = relay@left;
    let right_after = relay@right;
    let copied = answer;
    left_after;
    right_after;
    copied;
    unit
}
