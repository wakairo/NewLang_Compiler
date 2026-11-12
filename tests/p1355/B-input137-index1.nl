struct Frame { payload:u8, tag:u8 }
fn spin(input:u8) -> u8 {
    let inner = Frame { payload:u8(179), tag:u8(19) };
    let loop_old = loop(phase=usize(0)) {
        if (phase < usize(3)) {
            let previous = loan_write(inner@tag) { |slot|
                replace(slot,input)
            };
            continue(phase+usize(1));
        } else {
            let old = loan_write(inner@tag) { |slot|
                replace(slot,u8(239))
            };
            break old;
        }
    };
    let current = inner@tag;
    let sibling = inner@payload;
    current;
    sibling;
    let copied = loop_old;
    copied
}
fn relay(outer:u8) -> u8 {
    let original = Frame { payload:outer, tag:u8(61) };
    let argument = original@payload;
    let answer = spin(argument);
    let displaced = loan_write(original@tag) { |slot|
        replace(slot,answer)
    };
    let current = original@tag;
    let sibling = original@payload;
    displaced;
    current;
    sibling;
    let copied = answer;
    copied
}
fn main() -> unit {
    let supplied = u8(137);
    let result = relay(supplied);
    let held = result;
    held;
    unit
}
