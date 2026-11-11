struct Meter { badge:u8, reading:u8 }
fn adjust(stamp:usize) -> u8 {
    if (usize(1) < stamp) { u8(67) } else { u8(199) }
}
fn main() -> unit {
    let meter = Meter { badge:u8(211), reading:u8(37) };
    let receipt = loop(round=usize(0)) {
        if (round < usize(2)) {
            let next = adjust(round);
            let prior = loan_write(meter@reading) { |slot|
                replace(slot,next)
            };
            continue(round+usize(1));
        } else {
            let computed = adjust(round);
            let kept = computed;
            let old = loan_write(meter@reading) { |slot|
                replace(slot,kept)
            };
            break old;
        }
    };
    let current = meter@reading;
    let sibling = meter@badge;
    let copied = receipt;
    current;
    sibling;
    copied;
    unit
}
