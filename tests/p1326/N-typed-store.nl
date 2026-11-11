struct Dial { setting:u8, seal:u8 }
fn tune(phase:usize) -> u8 {
    if (phase < usize(2)) { u8(83) } else { u8(227) }
}
fn main() -> unit {
    let dial = Dial { setting:u8(29), seal:u8(173) };
    let result = loop(phase=usize(0)) {
        if (phase < usize(2)) {
            let next = tune(phase);
            let previous = loan_write(dial@setting) { |slot|
                replace(slot,next)
            };
            continue(phase+usize(1));
        } else {
            let terminal_new = tune(phase);
            let terminal_copy = terminal_new;
            let old = loan_write(dial@setting) { |slot|
                replace(slot,usize(227))
            };
            break old;
        }
    };
    let after = dial@setting;
    let sibling = dial@seal;
    let copied = result;
    after;
    sibling;
    copied;
    unit
}
