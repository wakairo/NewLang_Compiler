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
    let incoming = original@stamp;
    let returned = fold(usize(137));
    let old_caller = loan_write(original@datum) { |slot|
        replace(slot,returned)
    };
    let current = original@datum;
    let sibling = original@stamp;
    old_caller;
    current;
    sibling;
    unit
}
