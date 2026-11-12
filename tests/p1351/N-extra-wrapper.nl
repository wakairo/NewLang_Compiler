struct Gauge { live:u8, mark:u8 }
fn cycle(seed:u8) -> u8 {
    let inner = Gauge { live:u8(17), mark:u8(193) };
    let loop_old = loop(phase=usize(0)) {
        if (phase < usize(2)) {
            let prior = loan_write(inner@live) { |slot|
                replace(slot,seed)
            };
            continue(phase+usize(1));
        } else {
            let old = loan_write(inner@live) { |slot|
                replace(slot,u8(211))
            };
            break old;
        }
    };
    let current = inner@live;
    let sibling = inner@mark;
    current;
    sibling;
    let copied = loop_old;
    copied
}
fn wrap() -> u8 {
    let original = Gauge { live:u8(41), mark:u8(167) };
    let input = original@mark;
    let answer = cycle(input);
    let displaced = loan_write(original@live) { |slot|
        replace(slot,answer)
    };
    let current = original@live;
    let sibling = original@mark;
    displaced;
    current;
    sibling;
    let copied = answer;
    copied
}
fn outer() -> u8 {
    let answer = wrap();
    answer
}
fn main() -> unit {
    let result = outer();
    let held = result;
    held;
    unit
}
