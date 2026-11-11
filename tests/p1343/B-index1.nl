struct Cell { datum:u8, stamp:u8 }
fn fold(incoming:u8) -> u8 {
    let scratch = Cell { datum:u8(157), stamp:u8(37) };
    let loop_old = loop(phase=usize(0)) {
        if (phase < usize(3)) {
            let previous = loan_write(scratch@stamp) { |slot|
                replace(slot,incoming)
            };
            continue(phase+usize(1));
        } else {
            let old = loan_write(scratch@stamp) { |slot|
                replace(slot,u8(211))
            };
            break old;
        }
    };
    let current = scratch@stamp;
    let sibling = scratch@datum;
    current;
    sibling;
    let copied = loop_old;
    copied
}
fn main() -> unit {
    let original = Cell { datum:u8(103), stamp:u8(47) };
    let incoming = original@datum;
    let returned = fold(incoming);
    let old_caller = loan_write(original@stamp) { |slot|
        replace(slot,returned)
    };
    let current = original@stamp;
    let sibling = original@datum;
    old_caller;
    current;
    sibling;
    unit
}
