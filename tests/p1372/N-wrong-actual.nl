struct Meter { reserve:u8, active:u8 }
fn take_local_old(input:u8) -> u8 {
    let ephemeral = Meter { reserve:u8(109), active:input };
    let before = ephemeral@active;
    let old = loan_write(ephemeral@active) { |slot|
        replace(slot,u8(227))
    };
    let current = ephemeral@active;
    let sibling = ephemeral@reserve;
    before;
    current;
    sibling;
    let copied = old;
    copied
}
fn main() -> unit {
    let original = Meter { reserve:u8(173), active:u8(47) };
    let rejected = take_local_old(usize(1));
    let result = loop(phase=usize(0)) {
        if (phase < usize(3)) {
            let present = original@active;
            let carried = take_local_old(present);
            let displaced = loan_write(original@active) { |slot|
                replace(slot,carried)
            };
            continue(phase+usize(1));
        } else {
            let old = loan_write(original@active) { |slot|
                replace(slot,u8(239))
            };
            break old;
        }
    };
    let current = original@active;
    let sibling = original@reserve;
    result;
    current;
    sibling;
    unit
}
