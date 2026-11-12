struct Frame { payload:u8, tag:u8 }
fn spin(input:u8) -> u8 {
    let inner = Frame { payload:u8(13), tag:u8(191) };
    let loop_old = loop(phase=usize(0)) {
        if (phase < usize(2)) {
            let previous = loan_write(inner@payload) { |slot|
                replace(slot,input)
            };
            continue(phase+usize(1));
        } else {
            let old = loan_write(inner@payload) { |slot|
                replace(slot,u8(227))
            };
            break old;
        }
    };
    let current = inner@payload;
    let sibling = inner@tag;
    current;
    sibling;
    let copied = loop_old;
    copied
}
fn relay(outer:u8) -> u8 {
    let original = Frame { payload:u8(47), tag:outer };
    let argument = original@tag;
    let answer = spin(argument);
    let displaced = loan_write(original@payload) { |slot|
        replace(slot,answer)
    };
    let current = original@payload;
    let sibling = original@tag;
    displaced;
    current;
    sibling;
    let copied = answer;
    copied
}
fn main() -> unit {
    let supplied = usize(83);
    let result = relay(supplied);
    let held = result;
    held;
    unit
}
