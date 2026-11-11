struct Cell { datum:u8, stamp:u8 }
fn fold(incoming:u8) -> u8 {
    let scratch = Cell { datum:u8(29), stamp:u8(173) };
    let loop_old = loop(phase=usize(0)) {
        if (phase < usize(2)) {
            let previous = loan_write(scratch@datum) { |slot|
                replace(slot,incoming)
            };
            continue(phase+usize(1));
        } else {
            let old = loan_write(scratch@datum) { |slot|
                replace(slot,u8(229))
            };
            break old;
        }
    };
    let current = scratch@datum;
    let sibling = scratch@stamp;
    current;
    sibling;
    let copied = loop_old;
    copied
}
fn main() -> unit {
    let original = Cell { datum:u8(61), stamp:u8(137) };
    let extra = Cell { datum:u8(7), stamp:u8(193) };
    extra;
    let input_first = original@stamp;
    let first = fold(input_first);
    let kept_first = first;
    let caller_old_first = loan_write(original@datum) { |slot|
        replace(slot,first)
    };
    let input_second = original@datum;
    let second = fold(input_second);
    let copied_second = second;
    let caller_old_after_second = loan_write(original@stamp) { |slot|
        replace(slot,kept_first)
    };
    let current = original@datum;
    let sibling = original@stamp;
    caller_old_first;
    copied_second;
    caller_old_after_second;
    current;
    sibling;
    kept_first;
    unit
}
