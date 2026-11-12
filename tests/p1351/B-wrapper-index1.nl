struct Gauge { live:u8, mark:u8 }
fn cycle(seed:u8) -> u8 {
    let inner = Gauge { live:u8(181), mark:u8(23) };
    let loop_old = loop(phase=usize(0)) {
        if (phase < usize(3)) {
            let prior = loan_write(inner@mark) { |slot|
                replace(slot,seed)
            };
            continue(phase+usize(1));
        } else {
            let old = loan_write(inner@mark) { |slot|
                replace(slot,u8(239))
            };
            break old;
        }
    };
    let current = inner@mark;
    let sibling = inner@live;
    current;
    sibling;
    let copied = loop_old;
    copied
}
fn wrap() -> u8 {
    let original = Gauge { live:u8(149), mark:u8(59) };
    let input = original@live;
    let answer = cycle(input);
    let displaced = loan_write(original@mark) { |slot|
        replace(slot,answer)
    };
    let current = original@mark;
    let sibling = original@live;
    displaced;
    current;
    sibling;
    let copied = answer;
    copied
}
fn main() -> unit {
    let result = wrap();
    let held = result;
    held;
    unit
}
